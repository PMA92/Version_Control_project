# Mini Git - Version Control System in C

This project is a simplified version control system written in C that mimics some of the core features of Git. It allows users to initialize a repository, stage files, commit changes, manage branches, and check the status of the working directory.

## Features

- Initialize a `.mockgit` repository
- Add (stage) files
- Commit staged files
- Log commit history
- Check file status (staged, modified, untracked)
- Create branches
- Checkout branches or specific commits
- Supports attached and detached HEAD states
- Stores content as hashed blobs (SHA-256)

## Commands

### Initialize Repository

mockgit init

# Add files to the stage

mockgit add <filename> [filename2 ...]

# Make commits

mockgit commit -m "<commit message>"

# View commit log

mockgit log

# Check status

mockgit status

# Create branch

mockgit branch <branch-name>

# Checkout to a branch or commit

mockgit checkout <branch-name|commit-hash>
