# Show file notes (full content)
gitnote show file path/to/file.c

# Show directory notes (just that dir)
gitnote show dir src/

# Show ALL notes in directory tree
gitnote show dir src/ --recursive
gitnote show dir src/ -r  # short form

# Show commit notes
gitnote show commit HEAD

# After git pull (automatic)
git pull
# → Automatically displays commit notes if they exist
