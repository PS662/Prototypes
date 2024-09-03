
# Bitcask Dictionary

A simple dictionary using the Bitcask storage format with versioning and REST API support.

- Create and update dictionaries using CSV files.
- Search words via a Flask API.
- Versioning of dictionaries (e.g., `dictionary_1.bitcask`).


### Compile the Program

```bash
make
```

### Create a Dictionary

```bash
./bitcask_dictionary --create-dict words.csv
```

### Update Dictionary

```bash
./bitcask_dictionary --merge-dict updated_dict.bitcask
```

## Configuration

Edit `dictionary.config` to set the dictionary path and version.

## Clean Up

```bash
make clean
```

## C++ CLI Options

- `--create-dict <csv>`: Creates a dictionary from the provided CSV file.
- `--merge-dict <changelog>`: Updates the dictionary using the changelog CSV file.
- `--search <word>`: Searches for the word in the dictionary.

## Makefile Commands

- `make`: Compiles the C++ source into an executable.
- `make clean`: Removes the executable.