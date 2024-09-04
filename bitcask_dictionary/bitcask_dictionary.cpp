#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <condition_variable>
#include <csignal>
#include <iomanip>
#include <algorithm>

std::string dictPath;
std::string version;
std::string configPath = "dictionary.config";

#pragma pack(push, 1)
struct BitcaskHeader
{
    uint32_t version;     // Version number
    uint64_t indexOffset; // Offset where the index section starts
    uint64_t dataOffset;  // Offset where the data section starts
    uint32_t entryCount;  // Number of entries in the dictionary

    void WriteToFile(std::ofstream &out)
    {
        out.write(reinterpret_cast<const char *>(&version), sizeof(version));
        out.write(reinterpret_cast<const char *>(&indexOffset), sizeof(indexOffset));
        out.write(reinterpret_cast<const char *>(&dataOffset), sizeof(dataOffset));
        out.write(reinterpret_cast<const char *>(&entryCount), sizeof(entryCount));
    }

    void ReadFromFile(std::ifstream &in)
    {
        in.read(reinterpret_cast<char *>(&version), sizeof(version));
        in.read(reinterpret_cast<char *>(&indexOffset), sizeof(indexOffset));
        in.read(reinterpret_cast<char *>(&dataOffset), sizeof(dataOffset));
        in.read(reinterpret_cast<char *>(&entryCount), sizeof(entryCount));
    }
};
#pragma pack(pop)

// Function to calculate a simple checksum
uint32_t CalculateChecksum(const std::string &data)
{
    uint32_t checksum = 0;
    for (char c : data)
    {
        checksum += c;
    }
    return checksum;
}

// Helper function to write a Bitcask entry
void WriteBitcaskEntry(std::ofstream &out, const std::string &word, const std::string &meaning)
{
    uint32_t checksum = CalculateChecksum(word + meaning);
    uint32_t wordSize = word.size();
    uint32_t meaningSize = meaning.size();

    // Write entry in the format: [checksum][wordSize][meaningSize][word][meaning]
    out.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum));
    std::cout << "Write checksum (" << checksum << ") for word: '" << word << "', size: " << sizeof(checksum) << " bytes\n";

    out.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
    std::cout << "Write word size (" << wordSize << ") for word: '" << word << "', size: " << sizeof(wordSize) << " bytes\n";

    out.write(reinterpret_cast<const char *>(&meaningSize), sizeof(meaningSize));
    std::cout << "Write meaning size (" << meaningSize << ") for word: '" << word << "', size: " << sizeof(meaningSize) << " bytes\n";

    out.write(word.c_str(), wordSize);
    std::cout << "Write word: '" << word << "', size: " << wordSize << " bytes\n";

    out.write(meaning.c_str(), meaningSize);
    std::cout << "Write meaning for word: '" << word << "', size: " << meaningSize << " bytes\n";
}

void CreateDictionary(const std::string &csvFilePath, const std::string &bitcaskFilePath)
{
    std::ifstream inFile(csvFilePath);
    std::ofstream outFile(bitcaskFilePath, std::ios::binary);

    BitcaskHeader header = {static_cast<uint32_t>(std::stoi(version)), 0, 0, 0}; // Initialize header with defaults
    outFile.seekp(sizeof(BitcaskHeader));                                        // Reserve space for the header

    std::cout << "Reserved space for header: " << sizeof(BitcaskHeader) << " bytes\n";

    std::string line;
    std::vector<std::pair<std::string, uint64_t>> index; // For storing index entries
    uint64_t dataStart = outFile.tellp();

    std::cout << "Data section starts at offset: " << dataStart << "\n";

    // Read the CSV and write data entries
    while (std::getline(inFile, line))
    {
        std::istringstream ss(line);
        std::string word, meaning;
        if (std::getline(ss, word, ',') && std::getline(ss, meaning))
        {
            uint64_t currentOffset = static_cast<uint64_t>(outFile.tellp());
            index.push_back({word, currentOffset}); // Store the index
            std::cout << "Writing entry for word: '" << word << "' at offset: " << currentOffset << "\n";
            WriteBitcaskEntry(outFile, word, meaning);
            header.entryCount++;
        }
    }

    uint64_t indexStart = outFile.tellp();
    std::cout << "Index section starts at offset: " << indexStart << "\n";

    // Write the index section
    for (const auto &entry : index)
    {
        uint32_t wordSize = entry.first.size();
        outFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
        outFile.write(entry.first.c_str(), wordSize);
        outFile.write(reinterpret_cast<const char *>(&entry.second), sizeof(entry.second)); // Offset
        std::cout << "Index entry for word: '" << entry.first << "', offset: " << entry.second << "\n";
    }

    // Update header with correct offsets
    header.indexOffset = indexStart;
    header.dataOffset = dataStart;
    outFile.seekp(0); // Go back to the start to write the header
    header.WriteToFile(outFile);

    std::cout << "Header updated with data offset: " << header.dataOffset
              << ", index offset: " << header.indexOffset
              << ", entry count: " << header.entryCount << "\n";

    inFile.close();
    outFile.close();
}

void ReadDictionary(const std::string &bitcaskFilePath)
{
    std::ifstream inFile(bitcaskFilePath, std::ios::binary);
    if (!inFile)
    {
        std::cerr << "Failed to open file: " << bitcaskFilePath << "\n";
        return;
    }

    // Read the header
    BitcaskHeader header;
    header.ReadFromFile(inFile);

    std::cout << "Read Header:\n";
    std::cout << "  Version: " << header.version << "\n";
    std::cout << "  Entry Count: " << header.entryCount << "\n";
    std::cout << "  Data Offset: " << header.dataOffset << "\n";
    std::cout << "  Index Offset: " << header.indexOffset << "\n";

    // Read and process each index entry without loading them into memory
    inFile.seekg(header.indexOffset); // Move to the index section
    std::cout << "Reading Index Entries and Corresponding Data:\n";

    for (uint32_t i = 0; i < header.entryCount; ++i)
    {
        uint32_t wordSize;
        inFile.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));

        std::string word(wordSize, '\0');
        inFile.read(&word[0], wordSize);

        uint64_t offset;
        inFile.read(reinterpret_cast<char *>(&offset), sizeof(offset));

        std::cout << "  Index Entry: Word: '" << word << "', Offset: " << offset << "\n";

        // Now read the data entry at the given offset
        std::streampos currentPos = inFile.tellg(); // Save current position
        inFile.seekg(offset);                       // Move to the word's data section

        uint32_t checksum;
        inFile.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));

        uint32_t wordSizeInData;
        inFile.read(reinterpret_cast<char *>(&wordSizeInData), sizeof(wordSizeInData));

        uint32_t meaningSize;
        inFile.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));

        std::string wordInData(wordSizeInData, '\0');
        inFile.read(&wordInData[0], wordSizeInData);

        std::string meaning(meaningSize, '\0');
        inFile.read(&meaning[0], meaningSize);

        std::cout << "  Data Entry: Word: '" << wordInData << "', Checksum: " << checksum << "\n";
        std::cout << "  Meaning: '" << meaning << "'\n";
        std::cout << "  Word Size: " << wordSizeInData << ", Meaning Size: " << meaningSize << "\n";

        inFile.seekg(currentPos); // Return to the position after reading the index entry
    }

    inFile.close();
}

std::pair<uint64_t, uint32_t> FindWordInBitcask(const std::string &word, std::ifstream &inFile, const BitcaskHeader &header)
{
    // Go to the index section as specified by the header
    inFile.seekg(header.indexOffset);
    std::cout << "Reading index section starting at offset: " << header.indexOffset << std::endl;

    // Iterate through each entry in the index section
    for (uint32_t i = 0; i < header.entryCount; ++i)
    {
        // Read the size of the word
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

        std::cout << "Stored word: '" << storedWord << "' vs. Search word: '" << word << "'" << std::endl;

        // Compare the stored word with the search word
        if (storedWord == word)
        {
            // Read data offset
            uint64_t dataOffset;
            inFile.read(reinterpret_cast<char *>(&dataOffset), sizeof(dataOffset));
            if (inFile.fail())
            {
                std::cerr << "Error reading data offset from index." << std::endl;
                break;
            }
            // Seek directly to the data offset found in the index entry
            inFile.seekg(dataOffset, std::ios::beg);
            if (!inFile)
            {
                std::cerr << "Error seeking to data offset: " << dataOffset << std::endl;
                break;
            }

            // Read the entire data entry using the known structure
            uint32_t checksum, readWordSize, meaningSize;
            inFile.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));
            inFile.read(reinterpret_cast<char *>(&readWordSize), sizeof(readWordSize));
            inFile.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));
            if (inFile.fail())
            {
                std::cerr << "Error reading data entry details." << std::endl;
                break;
            }

            // Skip reading the word since we already know it, just move the read pointer
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

    // If the word is not found
    std::cout << "We finally get here! Word not found: " << word << std::endl;
    return {0, 0}; // Not found
}

void SearchWord(const std::string &word, const std::string &searchDictPath)
{
    std::ifstream inFile(searchDictPath, std::ios::binary);
    if (!inFile.is_open())
    {
        std::cout << "Failed to open dictionary file." << std::endl;
        return;
    }

    BitcaskHeader header;
    header.ReadFromFile(inFile); // Read the header to get index and data offsets

    auto [pos, meaningSize] = FindWordInBitcask(word, inFile, header);
    if (pos != 0)
    {
        inFile.seekg(pos); // Go to meaning position
        std::string meaning(meaningSize, '\0');
        inFile.read(&meaning[0], meaningSize);
        std::cout << word << ": " << meaning << std::endl;
    }
    else
    {
        std::cout << "Word not found: " << word << std::endl;
    }

    inFile.close();
}

void MergeDictionary(const std::string &dict1Path, const std::string &dict2Path, const std::string &outputDictPath)
{
    std::ifstream dict1(dict1Path, std::ios::binary);
    std::ifstream dict2(dict2Path, std::ios::binary);
    std::ofstream mergedFile(outputDictPath, std::ios::binary);

    if (!dict1.is_open() || !dict2.is_open() || !mergedFile.is_open())
    {
        std::cerr << "Error opening one of the Bitcask files." << std::endl;
        return;
    }

    // Read headers from both dictionaries
    BitcaskHeader header1, header2;
    header1.ReadFromFile(dict1);
    header2.ReadFromFile(dict2);

    // Reserve space for the header in the merged file
    BitcaskHeader mergedHeader = {header1.version + 1, 0, 0, 0}; // Increment version for the merged dictionary
    mergedFile.seekp(sizeof(BitcaskHeader));                     // Reserve space for the header
    std::cout << "Reserved space for header: " << sizeof(BitcaskHeader) << " bytes" << std::endl;

    uint64_t dataStart = mergedFile.tellp(); // Data section start
    std::cout << "Data section starts at: " << dataStart << std::endl;

    // Set up reading from the index sections of both dictionaries
    dict1.seekg(header1.indexOffset);
    dict2.seekg(header2.indexOffset);

    // Storage for the merged index
    std::vector<std::pair<std::string, uint64_t>> index;

    // Read the first entries from both dictionaries
    std::string word1, word2;
    uint64_t offset1 = 0, offset2 = 0;
    bool valid1 = header1.entryCount > 0; // Flags to check validity
    bool valid2 = header2.entryCount > 0;

    if (valid1)
    {
        uint32_t wordSize1;
        dict1.read(reinterpret_cast<char *>(&wordSize1), sizeof(wordSize1));
        word1.resize(wordSize1);
        dict1.read(&word1[0], wordSize1);
        dict1.read(reinterpret_cast<char *>(&offset1), sizeof(offset1));
        valid1 = dict1.good();
    }

    if (valid2)
    {
        uint32_t wordSize2;
        dict2.read(reinterpret_cast<char *>(&wordSize2), sizeof(wordSize2));
        word2.resize(wordSize2);
        dict2.read(&word2[0], wordSize2);
        dict2.read(reinterpret_cast<char *>(&offset2), sizeof(offset2));
        valid2 = dict2.good();
    }

    // Continue reading while at least one dictionary has valid entries
    while (valid1 || valid2)
    {
        if (valid1 && (!valid2 || (valid2 && word1 < word2)))
        {
            // Write the entry from the first dictionary
            uint64_t currentOffset = mergedFile.tellp(); // Store the current offset before writing
            std::streampos currentPos = dict1.tellg();   // Save current position
            dict1.seekg(offset1);                        // Move to the word's data section in dict1

            uint32_t checksum, wordSize, meaningSize;
            dict1.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));
            dict1.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));
            dict1.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));

            std::string word(wordSize, '\0');
            std::string meaning(meaningSize, '\0');
            dict1.read(&word[0], wordSize);
            dict1.read(&meaning[0], meaningSize);

            mergedFile.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum));
            mergedFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
            mergedFile.write(reinterpret_cast<const char *>(&meaningSize), sizeof(meaningSize));
            mergedFile.write(word.c_str(), wordSize);
            mergedFile.write(meaning.c_str(), meaningSize);

            // Store the index entry in memory
            index.push_back({word1, currentOffset});
            mergedHeader.entryCount++;

            dict1.seekg(currentPos); // Return to position after reading the index entry
            uint32_t wordSize1;
            if (dict1.read(reinterpret_cast<char *>(&wordSize1), sizeof(wordSize1)))
            {
                word1.resize(wordSize1);
                dict1.read(&word1[0], wordSize1);
                dict1.read(reinterpret_cast<char *>(&offset1), sizeof(offset1));
                valid1 = dict1.good();
            }
            else
            {
                valid1 = false; // No more entries in dict1
            }
        }
        else if (valid2 && (!valid1 || (valid1 && word2 < word1)))
        {
            // Write the entry from the second dictionary
            uint64_t currentOffset = mergedFile.tellp(); // Store the current offset before writing
            std::streampos currentPos = dict2.tellg();   // Save current position
            dict2.seekg(offset2);                        // Move to the word's data section in dict2

            uint32_t checksum, wordSize, meaningSize;
            dict2.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));
            dict2.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));
            dict2.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));

            std::string word(wordSize, '\0');
            std::string meaning(meaningSize, '\0');
            dict2.read(&word[0], wordSize);
            dict2.read(&meaning[0], meaningSize);

            mergedFile.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum));
            mergedFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
            mergedFile.write(reinterpret_cast<const char *>(&meaningSize), sizeof(meaningSize));
            mergedFile.write(word.c_str(), wordSize);
            mergedFile.write(meaning.c_str(), meaningSize);

            // Store the index entry in memory
            index.push_back({word2, currentOffset});
            mergedHeader.entryCount++;

            dict2.seekg(currentPos); // Return to position after reading the index entry
            uint32_t wordSize2;
            if (dict2.read(reinterpret_cast<char *>(&wordSize2), sizeof(wordSize2)))
            {
                word2.resize(wordSize2);
                dict2.read(&word2[0], wordSize2);
                dict2.read(reinterpret_cast<char *>(&offset2), sizeof(offset2));
                valid2 = dict2.good();
            }
            else
            {
                valid2 = false; // No more entries in dict2
            }
        }
        else if (valid1 && valid2 && word1 == word2)
        {
            // Replace the entry from the first dictionary with the second dictionary's entry
            uint64_t currentOffset = mergedFile.tellp();    // Store the current offset before writing
            std::streampos currentPosDict1 = dict1.tellg(); // Save current position in dict1
            std::streampos currentPosDict2 = dict2.tellg(); // Save current position in dict2
            dict2.seekg(offset2);                           // Prefer the second dictionary's data

            uint32_t checksum, wordSize, meaningSize;
            dict2.read(reinterpret_cast<char *>(&checksum), sizeof(checksum));
            dict2.read(reinterpret_cast<char *>(&wordSize), sizeof(wordSize));
            dict2.read(reinterpret_cast<char *>(&meaningSize), sizeof(meaningSize));

            std::string word(wordSize, '\0');
            std::string meaning(meaningSize, '\0');
            dict2.read(&word[0], wordSize);
            dict2.read(&meaning[0], meaningSize);

            mergedFile.write(reinterpret_cast<const char *>(&checksum), sizeof(checksum));
            mergedFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
            mergedFile.write(reinterpret_cast<const char *>(&meaningSize), sizeof(meaningSize));
            mergedFile.write(word.c_str(), wordSize);
            mergedFile.write(meaning.c_str(), meaningSize);

            // Store the index entry in memory
            index.push_back({word2, currentOffset});
            mergedHeader.entryCount++;

            dict1.seekg(currentPosDict1); // Return to position after reading the index entry in dict1
            dict2.seekg(currentPosDict2); // Return to position after reading the index entry in dict2

            uint32_t wordSize1, wordSize2;
            if (dict1.read(reinterpret_cast<char *>(&wordSize1), sizeof(wordSize1)))
            {
                word1.resize(wordSize1);
                dict1.read(&word1[0], wordSize1);
                dict1.read(reinterpret_cast<char *>(&offset1), sizeof(offset1));
                valid1 = dict1.good();
            }
            else
            {
                valid1 = false; // No more entries in dict1
            }

            if (dict2.read(reinterpret_cast<char *>(&wordSize2), sizeof(wordSize2)))
            {
                word2.resize(wordSize2);
                dict2.read(&word2[0], wordSize2);
                dict2.read(reinterpret_cast<char *>(&offset2), sizeof(offset2));
                valid2 = dict2.good();
            }
            else
            {
                valid2 = false; // No more entries in dict2
            }
        }
    }

    // Write the index section
    uint64_t indexStart = mergedFile.tellp();
    std::cout << "Index section starts at: " << indexStart << std::endl;

    for (const auto &entry : index)
    {
        uint32_t wordSize = entry.first.size();
        mergedFile.write(reinterpret_cast<const char *>(&wordSize), sizeof(wordSize));
        mergedFile.write(entry.first.c_str(), wordSize);
        mergedFile.write(reinterpret_cast<const char *>(&entry.second), sizeof(entry.second)); // Offset
        std::cout << "Wrote index entry for word: '" << entry.first << "' at offset: " << entry.second << std::endl;
    }

    // Update header with correct offsets and write it
    mergedHeader.indexOffset = indexStart;
    mergedHeader.dataOffset = dataStart;
    mergedHeader.entryCount = static_cast<uint32_t>(index.size());
    mergedFile.seekp(0);
    mergedHeader.WriteToFile(mergedFile);
    std::cout << "Header written with index offset: " << mergedHeader.indexOffset
              << ", data offset: " << mergedHeader.dataOffset
              << ", entry count: " << mergedHeader.entryCount << std::endl;

    dict1.close();
    dict2.close();
    mergedFile.close();

    // Update dictionary path and version in the config file
    dictPath = outputDictPath;
    version = std::to_string(mergedHeader.version);
    std::ofstream configOut(configPath);
    configOut << "path=" << dictPath << "\nversion=" << version << "\n";
    configOut.close();

    // Debug Output
    std::cout << "Merged Dictionary Size: " << std::filesystem::file_size(outputDictPath) << " bytes" << std::endl;
    std::cout << "Entries Merged: " << mergedHeader.entryCount << std::endl;
}

// Merges two large CSV files line by line, replacing old meanings with new ones
void MergeCSV(const std::string &csvFile1, const std::string &csvFile2, const std::string &outputCSV)
{
    std::ifstream file1(csvFile1);
    std::ifstream file2(csvFile2);
    std::ofstream mergedFile(outputCSV);

    if (!file1.is_open() || !file2.is_open() || !mergedFile.is_open())
    {
        std::cerr << "Error opening one of the CSV files." << std::endl;
        return;
    }

    std::string line1, line2;
    std::string word1, meaning1, word2, meaning2;

    auto parseCSVLine = [](const std::string &line, std::string &word, std::string &meaning) -> bool
    {
        std::istringstream ss(line);
        if (std::getline(ss, word, ',') && std::getline(ss, meaning))
        {
            return true;
        }
        return false;
    };

    // Initialize reading the first line of each file
    bool valid1 = std::getline(file1, line1) && parseCSVLine(line1, word1, meaning1);
    bool valid2 = std::getline(file2, line2) && parseCSVLine(line2, word2, meaning2);

    while (valid1 || valid2)
    {
        if (valid1 && (!valid2 || word1 < word2))
        {
            // Write the entry from the first CSV
            mergedFile << word1 << "," << meaning1 << "\n";
            valid1 = std::getline(file1, line1) && parseCSVLine(line1, word1, meaning1);
        }
        else if (valid2 && (!valid1 || word2 < word1))
        {
            // Write the entry from the second CSV
            mergedFile << word2 << "," << meaning2 << "\n";
            valid2 = std::getline(file2, line2) && parseCSVLine(line2, word2, meaning2);
        }
        else if (valid1 && valid2 && word1 == word2)
        {
            // Replace the entry from the first CSV with the second CSV's entry
            mergedFile << word2 << "," << meaning2 << "\n";
            valid1 = std::getline(file1, line1) && parseCSVLine(line1, word1, meaning1);
            valid2 = std::getline(file2, line2) && parseCSVLine(line2, word2, meaning2);
        }
    }

    file1.close();
    file2.close();
    mergedFile.close();
}

// Function to write the default configuration file
void CreateDefaultConfig()
{
    dictPath = "dictionary_1.bitcask";
    version = "1";
    std::ofstream configOut(configPath);
    configOut << "path=" << dictPath << "\nversion=" << version << "\n";
    configOut.close();
}

int main(int argc, char *argv[])
{
    // Try to read the config file
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
        std::cerr << "Config file not found. Creating default config.\n";
        CreateDefaultConfig(); // Create default config if not present
    }

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " --create-dict <csv> <output_path> | --search <word> | --update-dict <bitcask>\n";
        return 1;
    }

    std::string command = argv[1];
    if (command == "--create-dict" && (argc == 3 || argc == 4))
    {
        // Use output path from command line if provided, otherwise use config path
        std::string outputPath = (argc == 4) ? argv[3] : dictPath;
        CreateDictionary(argv[2], outputPath);
    }
    else if (command == "--search" && (argc == 3 || argc == 4))
    {
        std::string searchDictPath = (argc == 4) ? argv[3] : dictPath;
        SearchWord(argv[2], searchDictPath);
    }
    else if (command == "--merge-csv" && argc == 5)
    {
        MergeCSV(argv[2], argv[3], argv[4]);
    }
    else if (command == "--merge-dict" && argc == 5)
    {
        MergeDictionary(argv[2], argv[3], argv[4]);
    }
    else if (command == "--read-dict" && (argc == 2 || argc == 3))
    {
        std::string readDictPath = (argc == 3) ? argv[2] : dictPath;
        ReadDictionary(readDictPath);
    }
    else
    {
        std::cerr << "Invalid command or missing arguments.\n";
        return 1;
    }

    return 0;
}
