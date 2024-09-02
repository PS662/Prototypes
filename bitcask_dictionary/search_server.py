from flask import Flask, request, jsonify
import subprocess

app = Flask(__name__)

SEARCH_EXECUTABLE = "./bitcask_dictionary"

@app.route('/search', methods=['GET'])
def search_word():
    word = request.args.get('word', default='', type=str)
    if not word:
        return jsonify({"error": "Word parameter is required"}), 400
    
    result = subprocess.run([SEARCH_EXECUTABLE, '--search', word], capture_output=True, text=True)
    
    if result.returncode != 0:
        return jsonify({"error": "Failed to search for the word", "details": result.stderr}), 500
    
    meaning = result.stdout.strip()
    return f"{word}: {meaning}", 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
