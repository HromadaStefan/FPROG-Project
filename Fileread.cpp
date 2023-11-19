#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <range/v3/all.hpp>
#include <tuple>
#include <map>
#include <algorithm>
#include <numeric>
#include <future>

auto isChapterStart = [](const std::string &line)
{
    return line.find("CHAPTER") != std::string::npos;
};

auto addChapter = [](const std::vector<std::string> chapters, const std::string currentChapter)
{
    if (!currentChapter.empty())
    {
        auto updatedChapters = chapters;
        updatedChapters.push_back(std::move(currentChapter));
        return std::make_tuple(updatedChapters, std::string());
    }
    else
    {
        return std::make_tuple(chapters, currentChapter);
    }
};

auto getLines = [](const std::string filePath)
{
    std::ifstream file(filePath);
    std::vector<std::string> lines;

    if (file.is_open())
    {
        std::string line;
        while (getline(file, line))
        {
            lines.push_back(line);
        }
        file.close();
    }
    else
    {
        return std::make_tuple(false, lines);
    }

    return std::make_tuple(true, lines);
};

auto readFileIntoChapters = [](const std::string filePath)
{
    std::vector<std::string> chapters;
    std::string currentChapter;
    bool foundFirstChapter = false;

    auto getLinesResult = getLines(filePath);
    auto lines = std::get<1>(getLinesResult);

    if (std::get<0>(getLinesResult) == true)
    {
        ranges::for_each(lines, [&](const std::string &line)
                         {
            if (isChapterStart(line)) {
                if (foundFirstChapter) {
                    auto addChapterResult = addChapter(chapters, currentChapter);
                    chapters = std::get<0>(addChapterResult);
                    currentChapter = std::get<1>(addChapterResult);
                }
                foundFirstChapter = true;
            } else if (foundFirstChapter) {
                currentChapter += line + '\n';
            } });

        auto addChapterResult = addChapter(chapters, currentChapter);
        chapters = std::get<0>(addChapterResult);
        currentChapter = std::get<1>(addChapterResult);

    }
    else
    {
        return std::make_tuple(false, chapters);
    }

    return std::make_tuple(true, chapters);
};

// for 3) Tokenize text
auto tokenizeChapter = [](const std::string chapter) -> std::vector<std::string>
{
    std::vector<std::string> words;

    // Use a stringstream to split the chapter into words
    std::istringstream iss(chapter);
    ranges::copy(ranges::istream<std::string>(iss), ranges::back_inserter(words));

    // Remove punctuation and convert to lowercase using ranges::transform and lambdas
    ranges::transform(words, words.begin(), [](std::string &word)
                      {
        auto isPunctuation = [](char c) { return std::ispunct(c); };
        word.erase(ranges::remove_if(word, isPunctuation), word.end());
        ranges::transform(word, word.begin(), ::tolower);
        return word; });

    return words;
};

// for 3) Tokenize text

auto createWordList = [](const std::vector<std::string> &chaptersContent) -> std::vector<std::string>
{
    std::vector<std::string> wordList;

    // Iterate through each chapter using ranges::for_each
    ranges::for_each(chaptersContent, [&](const std::string &chapter)
                     {
        // Tokenize the current chapter and add words to the word list
        auto chapterWords = tokenizeChapter(chapter);
        wordList.insert(wordList.end(), chapterWords.begin(), chapterWords.end()); });

    return wordList;
};


// TODO ich versteh nicht warum das nicht funktioniert
//  for 4) Function to filter words from a list based on another list
auto filterWords = [](const std::vector<std::string> &wordsToFilter, const std::vector<std::string> &filterList)
{
    std::vector<std::string> filteredWords;

    std::vector<std::string> filter = filterList;

   std::copy_if(wordsToFilter.begin(), wordsToFilter.end(), std::back_inserter(filteredWords), [&](const std::string &word)
   {
        return std::any_of(filterList.begin(), filterList.end(), [&](const std::string &filterWord)
        {
            return word == filterWord;
        });
   });

    return filteredWords;
};

auto readTerms = [](const std::string filePath)
{
    std::vector<std::string> terms;

    // Open the file
    std::ifstream inputFile(filePath);

    // Check if the file is open
    if (inputFile.is_open())
    {
        std::string term;

        // Read each line from the file and add it to the vector
        while (std::getline(inputFile, term))
        {
            // hier war der Fehler für die Filterfunktion: Jedes element der terms hatte ein \n am ende
            term.pop_back();
            std::string copy = term;
            terms.push_back(copy);
        }

        // Close the file
        inputFile.close();
    }
    else
    {
        std::cerr << "Unable to open the file: " << filePath << std::endl;
    }

    return terms;
};

// for 5) Function to count word occurrences in a list
// Map function: takes a word and returns a key-value pair (word, 1)
auto MapOccurences = [](const std::string &word)
{
    return std::make_pair(word, 1);
};

// Reduce function: takes a key-value pair (word, [1, 1, ...]) and sums the values
auto ReduceOccurences = [](const std::map<std::string, std::vector<int>> &groupedData)
{
    std::map<std::string, int> result;
    ranges::for_each(groupedData, [&result](const auto &pair)
                     { result[pair.first] = std::accumulate(pair.second.begin(), pair.second.end(), 0); });
    return result;
};

// could not avoid using for loops here
auto parallelMap = [](const std::vector<std::string> &wordList)
{
    std::vector<std::vector<std::pair<std::string, int>>> partialMappedData;
    std::vector<std::pair<std::string, int>> mappedData;

    // Parallelize the Map step using std::async
    std::vector<std::future<void>> mapTasks;
    const size_t threadCount = 5;
    const size_t chunkSize = wordList.size() / threadCount;

    for (size_t i = 0; i < threadCount; ++i)
    {
        size_t startIdx = i * chunkSize;
        size_t endIdx = (i == threadCount - 1) ? wordList.size() : startIdx + chunkSize;

        mapTasks.emplace_back(std::async(
            std::launch::async, [&](size_t start, size_t end)
            {
            std::vector<std::pair<std::string, int>> partialData;
            std::transform(wordList.begin() + start, wordList.begin() + end, std::back_inserter(partialData), MapOccurences);
            partialMappedData.push_back(std::move(partialData)); },
            startIdx, endIdx));
    }

    // Wait for all mapTasks to complete
    for (auto &task : mapTasks)
    {
        task.wait();
    }

    // Combine the partial mapped data
    for (const auto &partialData : partialMappedData)
    {
        mappedData.insert(mappedData.end(), partialData.begin(), partialData.end());
    }

    return mappedData;
};

auto WordCountMapReduce = [](const std::vector<std::string> &wordList)
{
    // Map step
    std::vector<std::pair<std::string, int>> mappedData;
    mappedData = parallelMap(wordList);

    // Group the mapped data by key
    std::map<std::string, std::vector<int>> groupedData;
    ranges::for_each(mappedData, [&](const auto &pair)
                     { groupedData[pair.first].push_back(pair.second); });

    // Reduce step
    auto reducedData = ReduceOccurences(groupedData);

    return reducedData;
};

// Beispiel für die Verwendung der Funktion
int main()
{
    auto chapters = readFileIntoChapters("./txt-files/war_and_peace.txt");
    std::cout << std::get<1>(chapters).size() << " Kapitel" << std::endl;
    const std::vector<std::string> chaptersContent = std::get<1>(chapters);

    // Create the word list by processing each chapter
    std::vector<std::string> wordList = createWordList(chaptersContent);

    std::vector<std::string> peaceTerms = readTerms("./txt-files/peace_terms.txt");
    std::vector<std::string> warTerms = readTerms("./txt-files/war_terms.txt");

    std::vector<std::string> filteredPeace = filterWords(wordList, peaceTerms);
    std::vector<std::string> filteredWar = filterWords(wordList, warTerms);

    // Print the terms to verify

    std::cout << "PEACE FILTERED" << std::endl;
    for (const auto &t : filteredPeace)
    {
        std::cout << t << std::endl;
    }

    std::cout << "WAR FILTERED" << std::endl;
    for (const auto &t : filteredWar)
    {
        std::cout << t << std::endl;
    }

    // Parallelize the WordCountMapReduce function into 5 threads using std::async
    auto mapReduceTask = std::async(std::launch::async, [&]()
                                    { return WordCountMapReduce(filteredWar); });

    // Wait for the mapReduceTask to complete
    auto reducedData = mapReduceTask.get();

    /*
    // Print the result
    for (const auto& entry : reducedData) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }*/

    return 0;
}