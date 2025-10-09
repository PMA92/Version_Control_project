REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export PATH="$REPO_DIR:$PATH"

alias pit='mockgit'

echo "setup complete"