# Contributing to OpenGCC
OpenGCC aims to provide full-featured support for as many controllers as possible. Depending on if you're planning to contribute controller support, work on the core feature set, or make changes to documentation, follow the appropriate steps below.

## Contribution Instructions
### Controllers
1. Create a GitHub issue using the "Controller Support" template to track your work. This provides a place for discussion before a pull request is opened, and gives an easy overview of controllers with in-progress support.
2. Fork the repository
3. Make the necessary changes in the `controllers` directory
4. If changes to the `opengcc` directory are necessary, follow the steps for "[Features](#features)" below and merge those changes before attempting to merge controller support.
5. [Open a pull request](#opening-a-pull-request) from your fork to the main repository.

### Features
1. Create a GitHub issue using the "Feature" template to track your work. Discuss your proposed change in that issue if needed.
2. Fork the repository
3. Make the necessary changes in the `opengcc` directory
4. If changes to the `controllers` directory are necessary, include them in your feature branch.
5. [Open a pull request](#opening-a-pull-request) from your fork to the main repository.

### Documentation
1. Fork the repository
2. Make the necessary changes
3. [Open a pull request](#opening-a-pull-request) from your fork to the main repository.

## Opening a pull request
Link the related issue in your PR title, i.e. "[#Issue] PR Title"

## Merging Changes
Once your pull request is approved, squash your commits into one with the format "(PR Number) [Issue number] Short description."
