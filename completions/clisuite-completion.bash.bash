_clisuite_completions() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    local commands="init install-hooks verify repair todo note attach show sync pull reset help"
    
    case "${COMP_CWORD}" in
        1)
            COMPREPLY=($(compgen -W "${commands}" -- ${cur}))
            ;;
        2)
            case "${prev}" in
                todo)
                    COMPREPLY=($(compgen -W "add list done delete" -- ${cur}))
                    ;;
                note)
                    COMPREPLY=($(compgen -W "add edit list show delete search" -- ${cur}))
                    ;;
                attach)
                    COMPREPLY=($(compgen -W "commit file dir" -- ${cur}))
                    ;;
                show)
                    COMPREPLY=($(compgen -W "commit file dir" -- ${cur}))
                    ;;
                reset)
                    COMPREPLY=($(compgen -W "--tracked-only" -- ${cur}))
                    ;;
            esac
            ;;
        3)
            case "${COMP_WORDS[1]}" in
                attach|show)
                    case "${prev}" in
                        file|dir)
                            COMPREPLY=($(compgen -f -- ${cur}))
                            ;;
                        commit)
                            COMPREPLY=($(compgen -W "HEAD" -- ${cur}))
                            ;;
                    esac
                    ;;
            esac
            ;;
        *)
            case "${prev}" in
                --note)
                    local notes=$(clisuite note list 2>/dev/null | grep -o 'note_[0-9]*')
                    COMPREPLY=($(compgen -W "${notes}" -- ${cur}))
                    ;;
                -r|--recursive)
                    ;;
                *)
                    if [[ ${cur} == -* ]]; then
                        COMPREPLY=($(compgen -W "--note --recursive -r" -- ${cur}))
                    fi
                    ;;
            esac
            ;;
    esac
}

complete -F _clisuite_completions clisuite