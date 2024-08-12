#!/bin/bash

# Exit on any error
set -e

# Check if the script is run as root
if [ "$EUID" -ne 0 ]; then 
  echo "Please run as root"
  exit
fi

# Validate input arguments
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <database-name> <root-dir>"
  exit 1
fi

DB_NAME=$1
ROOT_DIR=$2

# Step 1: Prepare the directory
DATA_DIR="${ROOT_DIR}/${DB_NAME}"
mkdir -p "${DATA_DIR}"
chown mysql:mysql "${DATA_DIR}"
chmod 750 "${DATA_DIR}"

# Step 2: Update AppArmor
APPARMOR_PROFILE="/etc/apparmor.d/usr.sbin.mysqld"
TEMP_FILE="/tmp/apparmor_${DB_NAME}"

# Ensure the AppArmor profile exists
if [ ! -f "${APPARMOR_PROFILE}" ]; then
  echo "AppArmor profile does not exist: ${APPARMOR_PROFILE}"
  exit 1
fi

# Create a temporary file to work with
cp "${APPARMOR_PROFILE}" "${TEMP_FILE}"

# Add new rules before the last line (assuming the last line is a closing brace)
sed -i '$i\
# Configuration for '"$DB_NAME"'\
'"${DATA_DIR}"'/ r,\
'"${DATA_DIR}"'/** rwk,\
/var/run/mysqld/'"$DB_NAME"'.pid rw,\
/var/run/mysqld/'"$DB_NAME"'.sock rw,\
/var/run/mysqld/'"$DB_NAME"'.sock.lock rw,\
/run/mysqld/'"$DB_NAME"'.pid rw,\
/run/mysqld/'"$DB_NAME"'.sock rw,\
/run/mysqld/'"$DB_NAME"'.sock.lock rw,\
' "${TEMP_FILE}"

# Replace the original file with the modified file
mv "${TEMP_FILE}" "${APPARMOR_PROFILE}"

# Reload AppArmor configuration
apparmor_parser -r "${APPARMOR_PROFILE}"
systemctl restart apparmor

# Step 3: Initialize MySQL
mysqld --initialize --user=mysql --datadir="${DATA_DIR}"
echo "MySQL initialized for $DB_NAME at $DATA_DIR"

