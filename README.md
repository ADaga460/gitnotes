# Show file notes (full content)
clisuite show file path/to/file.c

# Show directory notes (just that dir)
clisuite show dir src/

# Show ALL notes in directory tree
clisuite show dir src/ --recursive
clisuite show dir src/ -r  # short form

# Show commit notes
clisuite show commit HEAD

# After git pull (automatic)
git pull
# → Automatically displays commit notes if they exist
