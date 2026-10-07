cmake_minimum_required(VERSION 3.21)
project(ArkWorldRetiredEntry LANGUAGES NONE)

# Keep the obsolete path explicit: all consumers share the root's Release libraries,
# runtime setup and test registration. There is no second simulation build graph.
message(FATAL_ERROR
    "The independent simulation entry is retired. Configure/build shared-libraries "
    "from the repository root, then use the headless-debug or headless-release preset "
    "and ctest --preset <preset> -L frozen. See docs/CONTRIBUTING.md.")
