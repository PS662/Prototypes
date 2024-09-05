
# Bitcask Dictionary

A simple dictionary using the Bitcask storage format with versioning and command-line support for various operations.

- Create and update dictionaries using CSV files.
- Search for words directly from the command line.
- Versioning of dictionaries (e.g., `dictionary_1.bitcask`).

## Compile the Program

```bash
make
```

## Create a Dictionary

To create a new dictionary from a CSV file:

```bash
./bitcask_dictionary --create-dict words.csv
```

You can also specify an output path for the new dictionary:

```bash
./bitcask_dictionary --create-dict changelog.csv replace_dict.bitcask
./bitcask_dictionary --create-dict additions.csv new_dict.bitcask
```

## Update (Merge) a Dictionary

To merge an existing dictionary with another Bitcask dictionary file. 
This will also update the default config with updated version number and dictionary path:

```bash
./bitcask_dictionary --merge-dict dictionary_1.bitcask new_dict.bitcask dictionary_2.bitcask
./bitcask_dictionary --merge-dict dictionary_2.bitcask replace_dict.bitcask dictionary_3.bitcask
```

## Search for a Word

Search for a word in a specified dictionary:

```bash
./bitcask_dictionary --search "banana" dictionary_1.bitcask
```

If no dictionary path is provided, it uses the path specified in the config file.

## Read a Dictionary

To read and print all entries from a dictionary:

```bash
./bitcask_dictionary --read-dict dictionary_1.bitcask
```

## CSV Helper Operations

Merge two CSV files into a single output CSV:

```bash
./bitcask_dictionary --merge-csv words.csv changelog.csv output.csv
```

## Configuration

The program reads configuration details from `dictionary.config`. If the config file is not found, a default config is created automatically with the following format:

```
path=dictionary_1.bitcask
version=1
```

This configuration file specifies the default dictionary path and version used by the application.

## Clean Up

To clean up compiled binaries and other generated files:

```bash
make clean
```

## C++ CLI Options

- `--create-dict <csv> [output_path]`: Creates a dictionary from the provided CSV file. Optionally specify an output path; otherwise, the path from the config file is used.
- `--merge-dict <dict1> <dict2> <output_path>`: Merges two dictionaries and saves the result to the specified output path.
- `--search <word> [dict_path]`: Searches for the word in the specified dictionary. Uses the config path if none is provided.
- `--read-dict [dict_path]`: Reads the dictionary at the given path and prints all entries. Uses the config path if none is provided.
- `--merge-csv <csv1> <csv2> <output_csv>`: Merges two CSV files into one output CSV.

## Makefile Commands

- `make`: Compiles the C++ source into an executable.
- `make clean`: Removes the executable.

## Default Config Creation

If the configuration file `dictionary.config` is missing, the program will automatically create a default config with a default dictionary path and version number.