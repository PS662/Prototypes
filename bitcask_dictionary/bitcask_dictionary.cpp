#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <condition_variable>
#include <csignal>
#include <iomanip>
#include <algorithm>

std::atomic<bool> running(true);
std::mutex dictMutex;
std::condition_variable updateNotifier;

std::string dictPath;
std::string version;
std::string configPath = "dictionary.config";

uint32_t calculateChecksum(const std::string &data)
{
    uint32_t checksum = 0;
    for (char c : data)
    {
        checksum += c;
    }
    return checksum;
}

struct BitcaskHeader
{
    uint32_t version;     // Version number
    uint64_t indexOffset; // Offset where the index section starts
    uint64_t dataOffset;  // Offset where the data section starts
    uint32_t entryCount;  // Number of entries in the dictionary

    void writeToFile(std::ofstream &out)
    {
        out.write(reinterpret_cast<const char *>(&version), sizeof(version));
        out.write(reinterpret_cast<const char *>(&indexOffset), sizeof(indexOffset));
        out.write(reinterpret_cast<const char *>(&dataOffset), sizeof(dataOffset));
        out.write(reinterpret_cast<const char *>(&entryCount), sizeof(entryCount));
    }

    void readFromFile(std::ifstream &in)
    {
        in.read(reinterpret_cast<char *>(&version), sizeof(version));
        in.read(reinterpret_cast<char *>(&indexOffset), sizeof(indexOffset));
        in.read(reinterpret_cast<char *>(&dataOffset), sizeof(dataOffset));
        in.read(reinterpret_cast<char *>(&entryCount), sizeof(entryCount));
    }
};

void writeBitcaskEntry(std::ofstream &out, const std::string &word, const std::string &meaning)
{
    uint32_t wordSize = word.size();
    uint32_t meaningSize = meaning.size();
    uint32_t checksum = calculateChecksum(word + meaning);

    // Format: [checksum][wordSize][meaningSize][word][meaning]
    out.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum));
    out.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
    out.write(reinterpret_cast<const char *>(&meaningSize), sizeof(meaningSize));
    out.write(word.c_str(), wordSize);
    out.write(meaning.c_str(), meaningSize);
}

void createDictionary(const std::string &csvFilePath, const std::string &bitcaskFilePath)
{
    std::ifstream inFile(csvFilePath);
    std::ofstream outFile(bitcaskFilePath, std::ios::binary);

    BitcaskHeader header = {std::stoi(version), 0, 0, 0}; // Initialize header with defaults
    outFile.seekp(sizeof(BitcaskHeader));                 // Reserve space for the header

    std::string line;
    std::vector<std::pair<std::string, uint64_t>> index; // For storing index entries
    uint64_t dataStart = outFile.tellp();

    // Read the CSV and write data entries
    while (std::getline(inFile, line))
    {
        std::istringstream ss(line);
        std::string word, meaning;
        if (std::getline(ss, word, ',') && std::getline(ss, meaning))
        {
            index.push_back({word, static_cast<uint64_t>(outFile.tellp())}); // Store the index
            writeBitcaskEntry(outFile, word, meaning);
            header.entryCount++;
        }
    }

    uint64_t indexStart = outFile.tellp();
    // Write the index section
    for (const auto &entry : index)
    {
        uint32_t wordSize = entry.first.size();
        outFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
        outFile.write(entry.first.c_str(), wordSize);
        outFile.write(reinterpret_cast<const char *>(&entry.second), sizeof(entry.second)); // Offset
    }

    // Update header with correct offsets
    header.indexOffset = indexStart;
    header.dataOffset = dataStart;
    outFile.seekp(0); // Go back to the start to write the header
    header.writeToFile(outFile);

    inFile.close();
    outFile.close();
}

std::pair<uint64_t, uint32_t> findWordInBitcask(const std::string &word, std::ifstream &inFile, const BitcaskHeader &header)
{
    // Go to the index section as specified by the header
    inFile.seekg(header.indexOffset);
    std::cout << "Reading index section starting at offset: " << header.indexOffset << std::endl;

    while (inFile.tellg() < static_cast<std::streampos>(header.indexOffset + (header.entryCount * (sizeof(uint32_t) + sizeof(uint64_t) + 256))))
    {
        // Read word size
        uint32_t wordSize;
        inFile.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));
        if (inFile.fail())
        {
            std::cerr << "Error reading word size from index." << std::endl;
            break;
        }

        // Read stored word
        std::string storedWord(wordSize, '\0');
        inFile.read(&storedWord[0], wordSize);
        if (inFile.fail())
        {
            std::cerr << "Error reading word from index." << std::endl;
            break;
        }

        // Read data offset
        uint64_t dataOffset;
        inFile.read(reinterpret_cast<char *>(&dataOffset), sizeof(dataOffset));
        if (inFile.fail())
        {
            std::cerr << "Error reading data offset from index." << std::endl;
            break;
        }

        std::cout << "Stored word: '" << storedWord << "' vs. Search word: '" << word << "'" << std::endl;

        // Compare the stored word with the search word
        if (storedWord == word)
        {
            // Seek to the data section using the data offset obtained from the index
            inFile.seekg(dataOffset, std::ios::beg);
            if (!inFile)
            {
                std::cerr << "Error seeking to data offset: " << dataOffset << std::endl;
                break;
            }

            // Read checksum, word size, and meaning size
            uint32_t checksum, readWordSize, meaningSize;
            inFile.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));
            inFile.read(reinterpret_cast<char *>(&readWordSize), sizeof(readWordSize));
            inFile.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));
            if (inFile.fail())
            {
                std::cerr << "Error reading data section details." << std::endl;
                break;
            }

            // Skip the actual word data since we already have the word
            inFile.seekg(readWordSize, std::ios::cur);
            if (!inFile)
            {
                std::cerr << "Error skipping word data in data section." << std::endl;
                break;
            }

            // Return the current position (start of the meaning) and the size of the meaning
            return {inFile.tellg(), meaningSize};
        }
    }

    std::cout << "We finally get here! Word not found: " << word << std::endl;
    return {0, 0}; // Not found
}

void searchWord(const std::string &word)
{
    std::ifstream inFile(dictPath, std::ios::binary);
    if (!inFile.is_open())
    {
        std::cout << "Failed to open dictionary file." << std::endl;
        return;
    }

    BitcaskHeader header;
    header.readFromFile(inFile); // Read the header to get index and data offsets

    auto [pos, meaningSize] = findWordInBitcask(word, inFile, header);
    if (pos != 0)
    {
        inFile.seekg(pos); // Go to meaning position
        std::string meaning(meaningSize, '\0');
        inFile.read(&meaning[0], meaningSize);
        std::cout << meaning << std::endl;
    }
    else
    {
        std::cout << "Word not found: " << word << std::endl;
    }

    inFile.close();
}

void mergeDictionaries(std::ifstream &dict1, std::ifstream &dict2, std::ofstream &mergedDict)
{
    std::string word1, meaning1, word2, meaning2;

    auto readNext = [](std::ifstream &file, std::string &word, std::string &meaning) -> bool
    {
        uint32_t checksum, wordSize, meaningSize;
        if (file.read(reinterpret_cast<char *>(&checksum), sizeof(checksum)))
        {
            file.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));
            file.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));
            word.resize(wordSize);
            meaning.resize(meaningSize);
            file.read(&word[0], wordSize);
            file.read(&meaning[0], meaningSize);
            return true;
        }
        return false;
    };

    bool valid1 = readNext(dict1, word1, meaning1);
    bool valid2 = readNext(dict2, word2, meaning2);

    while (valid1 || valid2)
    {
        if (valid1 && (!valid2 || word1 <= word2))
        {
            writeBitcaskEntry(mergedDict, word1, meaning1);
            valid1 = readNext(dict1, word1, meaning1);
        }
        else
        {
            writeBitcaskEntry(mergedDict, word2, meaning2);
            valid2 = readNext(dict2, word2, meaning2);
        }
    }
}

void updateDictionary(const std::string &changelogFilePath)
{
    std::string currentVersion = version;
    std::string newVersion = std::to_string(std::stoi(currentVersion) + 1);
    std::string newDictPath = "dictionary_" + newVersion + ".bitcask";

    std::string tempDictPath = "temp_dict.bitcask";
    createDictionary(changelogFilePath, tempDictPath);

    std::ifstream currentDict("dictionary_" + currentVersion + ".bitcask", std::ios::binary);
    std::ifstream tempFile(tempDictPath, std::ios::binary);
    std::ofstream mergedFile(newDictPath, std::ios::binary);

    BitcaskHeader header;
    header.readFromFile(currentDict); // Read header from the current dictionary

    currentDict.seekg(header.dataOffset);  // Move to the data section of the current dictionary
    tempFile.seekg(sizeof(BitcaskHeader)); // Skip header of temp

    mergeDictionaries(currentDict, tempFile, mergedFile);

    uint64_t indexStart = mergedFile.tellp();
    header.indexOffset = indexStart;
    header.dataOffset = sizeof(BitcaskHeader);
    header.entryCount = header.entryCount;  // Maintain the correct entry count
    header.version = std::stoi(newVersion); // Update the version number
    mergedFile.seekp(0);                    // Go back to the start to write the header
    header.writeToFile(mergedFile);

    currentDict.close();
    tempFile.close();
    mergedFile.close();

    // Update dictionary path and version in the config
    dictPath = newDictPath;
    version = newVersion;
    std::ofstream configOut(configPath);
    configOut << "path=" << dictPath << "\nversion=" << version << "\n";
    configOut.close();

    updateNotifier.notify_all();
}

void signalHandler(int signal)
{
    running = false;
    updateNotifier.notify_all();
}

int main(int argc, char *argv[])
{
    std::ifstream configIn(configPath);
    if (configIn.is_open())
    {
        std::getline(configIn, dictPath, '=');
        std::getline(configIn, dictPath);
        std::getline(configIn, version, '=');
        std::getline(configIn, version);
        configIn.close();
    }
    else
    {
        std::cerr << "Unable to open config file.\n";
        return 1;
    }

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --create-dict <csv> | --run | --update-dict <changelog> | --search <word>\n";
        return 1;
    }

    std::string command = argv[1];
    if (command == "--create-dict" && argc == 3)
    {
        createDictionary(argv[2], dictPath);
    }
    else if (command == "--run")
    {
        std::thread searchThread([]()
                                 {
            while (running) {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Keep thread alive
            } });
        searchThread.join();
    }
    else if (command == "--update-dict" && argc == 3)
    {
        std::thread updateThread(updateDictionary, argv[2]);
        updateThread.join();
    }
    else if (command == "--search" && argc == 3)
    {
        searchWord(argv[2]);
    }
    else
    {
        std::cerr << "Invalid command or missing arguments.\n";
        return 1;
    }

    signal(SIGINT, signalHandler);
    return 0;
}
