# Man pages with bat
set -gx MANROFFOPT "-c"
set -gx MANPAGER "sh -c 'col -bx | bat -l man -p'"

# Aliases
alias ls='eza -al --color=always --group-directories-first --icons'
alias la='eza -a --color=always --group-directories-first --icons'
alias ll='eza -l --color=always --group-directories-first --icons'
alias lt='eza -aT --color=always --group-directories-first --icons'
alias ..='cd ..'
alias ...='cd ../..'

# fd is installed as fdfind on Ubuntu
if command -q fdfind; and not command -q fd
    alias fd='fdfind'
end

function fish_greeting
    echo "🔧 Logitune Dev Container — type 'make help' for commands"
end


# Force multi-line prompt — override Pure's check function directly
function _pure_is_single_line_prompt
    return 1
end
