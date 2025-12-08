# **gitnote - Git-integrated note CLI**

_btw... this completely works. Anything else I push is additional features or efficiency._ 

_Technically you can push this in prod, but I'm implementing JSONL soon, so don't do that yet!_

# Usage

  gitnote init                              - Initialize gitnote in git repo
  
  gitnote install-hooks                     - Install git hooks
  
  gitnote verify                            - Check for orphaned attachments
  
  gitnote repair                            - Clean up orphaned attachments
  

# Notes (shared)
  gitnote note add "title" "content"        - Create note
  
  gitnote note edit <id> "title" "content"  - Edit note
  
  gitnote note list                         - List all notes
  
  gitnote note show <id>                    - Show note details
  
  gitnote note delete <id>                  - Delete note
  
  gitnote note search "query"               - Search notes

# Attach notes

  gitnote attach <path> "title" "content"   - Create & attach note (auto-detects file/dir)
  
  gitnote attach <path> --note <id>         - Attach existing note
  
  gitnote attach commit <hash> --note <id>  - Attach to commit
  

# View notes

  gitnote show file <path>                  - Show notes for file
  
  gitnote show dir <path> [--recursive]     - Show notes for directory
  
  gitnote show commit <hash>                - Show notes for commit
  

# Todos (private)

  gitnote todo add "task"                   - Add todo
  
  gitnote todo list                         - List todos
  
  gitnote todo done <id>                    - Mark todo done
  
  gitnote todo delete <id>                  - Delete todo
  

# Maintenance
  gitnote migrate <old-path> <new-path>     - Migrate attachments after file rename
  
  gitnote sync                              - Sync metadata to .gitnote/
  
  gitnote pull                              - Pull remote metadata
  
  gitnote reset [--tracked-only]            - Erase all gitnote data
  

# Examples
  **_Quick attach - create note and attach in one command_**
  
  gitnote attach src/main.c "Memory leak" "Line 45 leaks memory"
  
  gitnote attach src/ "TODO" "Refactor this module"


  **_Use existing note_**
  
  gitnote attach src/helper.c --note note_123456789


  **_View notes_**
  
  gitnote show file src/main.c
  
  gitnote show dir src/ --recursive
  


  **_Search and edit_**
  
  gitnote note search "memory"
  
  gitnote note edit note_123 "FIXED" "No longer leaks"
  
