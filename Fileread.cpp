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

auto tokenizeChapter(const std::vector<std::string> chapter) -> std::vector<std::string>
{
    std::vector<std::string> tokenizedChapter = chapter;

    ranges::transform(tokenizedChapter, tokenizedChapter.begin(), [](std::string& word) -> std::string {
        std::string newWord;
        for (char c : word) {
            if (!std::ispunct(c)) {
                newWord += c;
            }
        }
        ranges::transform(newWord, newWord.begin(), ::tolower);

        return newWord;
    });

    return tokenizedChapter;
};

auto read_file_by_words = [](const std::string& file_path) -> std::vector<std::vector<std::string>>
{
    std::vector<std::vector<std::string>> chapters;

    std::ifstream file (file_path);
    if(!file.is_open()) {
        std::cerr << "Datei konnte nicht geöffnet werden: " << file_path << '\n';
        return chapters;
    }

    auto words = ranges::istream_view<std::string>(file);
    bool write = false;
    std::vector<std::string> chapter;

    // Verwende ranges::for_each, um jede Zeile zu verarbeiten
    ranges::for_each(words, [&](const std::string& word)
    {
        if(word == "CHAPTER"){
            if(write == true){
                chapters.push_back(tokenizeChapter(chapter));
                chapter.clear();
            }
            else{
                write = true;
                chapter.clear();
            }
        } else{
            chapter.push_back(word);
        }
    });

    chapters.push_back(tokenizeChapter(chapter));
    chapter.clear();

    return chapters;
};

auto readTerms = [](const std::string& file_path) -> std::vector<std::string>
{
    std::vector<std::string> terms;

    std::ifstream file (file_path);
    if(!file.is_open()) {
        std::cerr << "Datei konnte nicht geöffnet werden: " << file_path << '\n';
        return terms;
    }

    auto words = ranges::istream_view<std::string>(file);

    ranges::for_each(words, [&](const std::string& word)
    {
        terms.push_back(word);
    });

    return terms;
};

auto checkIfExist = [] (const std::string word, const std::vector<std::string> &filterList)
{
    return std::any_of(filterList.begin(), filterList.end(), [&](const std::string &filterWord)
    {
        return word == filterWord;
    });
};

auto filterWords = [](const std::vector<std::string> &wordsToFilter, const std::vector<std::string> &filterList)
{
    std::vector<std::string> filteredWords;

    std::copy_if(wordsToFilter.begin(), wordsToFilter.end(), std::back_inserter(filteredWords), [&](const std::string &word)
    {
        return checkIfExist(word, filterList);
    });

    return filteredWords;
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
    std::vector<std::pair<std::string, int>> mappedData;

    // Parallelize the Map step using std::async
    std::vector<std::shared_future<void>> mapTasks;  // Use shared_future
    std::shared_ptr<std::vector<std::pair<std::string, int>>> sharedPartialMappedData = std::make_shared<std::vector<std::pair<std::string, int>>>();
    const size_t threadCount = 1;
    const size_t chunkSize = wordList.size() / threadCount;

    for (size_t i = 0; i < threadCount; ++i)
    {
        size_t startIdx = i * chunkSize;
        size_t endIdx = (i == threadCount - 1) ? wordList.size() : startIdx + chunkSize;

        mapTasks.emplace_back(std::async(
                std::launch::async, [&](size_t start, size_t end) {
                    std::vector<std::pair<std::string, int>> partialData;
                    std::transform(wordList.begin() + start, wordList.begin() + end, std::back_inserter(partialData), MapOccurences);
                    sharedPartialMappedData->insert(sharedPartialMappedData->end(), partialData.begin(), partialData.end());
                },
                startIdx, endIdx));
    }

    // Wait for all mapTasks to complete
    for (auto &task : mapTasks)
    {
        task.wait();
    }
    mappedData = *sharedPartialMappedData;

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


auto calculateTermDensity = [](const std::vector<std::string>& chapter, const std::vector<std::string>& filterWords) -> double
{
    double res = static_cast<double>(filterWords.size()) / chapter.size();
    return res;
};

auto calculateTermDistances = [](const std::vector<std::string>& chapter, const std::vector<std::string>& filterWords)
{
    auto filter_copy = filterWords;
    auto chapter_copy = chapter;

    int counter = 0;

    std::vector<int> distances;
    int index_temp = -1;

    ranges::for_each(chapter_copy, [&](std::string word)
    {
        if(checkIfExist(word, filterWords)){
            if(index_temp > -1) {
                distances.push_back(counter - index_temp);
            }
            index_temp = counter;
        }
        counter++;
    });

    return distances;
};

auto calculateAverageDistance = [](const std::vector<int> vec)
{
    if (vec.empty()) {
        return 0.0;
    }
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    double average = static_cast<double>(sum) / vec.size();

    return average;
};

auto combineDensityAndDistance = [](const double density, const double distance){
    return (density*10000 + distance/100);
};

auto printResult = [](const int index, const std::string value) {
    std::cout << "Chapter " << index << ": "<< value << std::endl<< std::flush;
};

int main()
{
    auto Chapters = read_file_by_words("./txt-files/war_and_peace.txt");

    std::cout << "Chapters: " << Chapters.size() << std::endl;

    auto peaceTerms = readTerms("./txt-files/peace_terms.txt");
    auto warTerms = readTerms("./txt-files/war_terms.txt");

    std::vector<std::string> filteredPeace;
    std::vector<std::string> filteredWar;

    std::vector<std::vector<std::string>> peaceFilteredChapters;
    std::vector<std::vector<std::string>> warFilteredChapters;

    std::vector<double> warChapterDensities;
    std::vector<double> peaceChapterDensities;

    std::vector<std::string> chapterRelations;

    ranges::for_each(Chapters, [&](const std::vector<std::string> chapter) {

        filteredPeace = filterWords(chapter, peaceTerms);
        filteredWar = filterWords(chapter, warTerms);
        peaceFilteredChapters.push_back(filteredPeace);
        warFilteredChapters.push_back(filteredWar);

        auto mapReduceTaskWar = std::async(std::launch::async, [&](){
            return WordCountMapReduce(filteredWar);
        });
        auto mapReduceTaskPeace = std::async(std::launch::async, [&](){
            return WordCountMapReduce(filteredPeace);
        });
        auto reducedDataWar = mapReduceTaskWar.get();
        auto reducedDataPeace = mapReduceTaskPeace.get();

        //calculate term density:
        double termDensityResultWar = calculateTermDensity(chapter, filteredWar);
        std::vector<int> termDistancesResultWar = calculateTermDistances(chapter, filteredWar);
        double combinationDensityDistanceWar = combineDensityAndDistance(termDensityResultWar, calculateAverageDistance(termDistancesResultWar));
        warChapterDensities.push_back(combinationDensityDistanceWar);

        double termDensityResultPeace = calculateTermDensity(chapter, filteredPeace);
        std::vector<int> termDistancesResultPeace = calculateTermDistances(chapter, filteredPeace);
        double combinationDensityDistancePeace = combineDensityAndDistance(termDensityResultPeace, calculateAverageDistance(termDistancesResultPeace));
        peaceChapterDensities.push_back(combinationDensityDistancePeace);

        if(combinationDensityDistancePeace>combinationDensityDistanceWar){
            chapterRelations.push_back("peace-related");
        }
        else if(combinationDensityDistanceWar>combinationDensityDistancePeace){
            chapterRelations.push_back("war-related");
        }
        else{
            chapterRelations.push_back("equal");
        }
    });

    ranges::for_each(chapterRelations, [index = 1](std::string value) mutable {
        printResult(index++, value);
    });
    
    return 0;
}