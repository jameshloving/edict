# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ubuntu/edict/query

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ubuntu/edict/query

# Include any dependencies generated for this target.
include CMakeFiles/query.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/query.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/query.dir/flags.make

CMakeFiles/query.dir/query.cpp.o: CMakeFiles/query.dir/flags.make
CMakeFiles/query.dir/query.cpp.o: query.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /home/ubuntu/edict/query/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/query.dir/query.cpp.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/query.dir/query.cpp.o -c /home/ubuntu/edict/query/query.cpp

CMakeFiles/query.dir/query.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/query.dir/query.cpp.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /home/ubuntu/edict/query/query.cpp > CMakeFiles/query.dir/query.cpp.i

CMakeFiles/query.dir/query.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/query.dir/query.cpp.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /home/ubuntu/edict/query/query.cpp -o CMakeFiles/query.dir/query.cpp.s

CMakeFiles/query.dir/query.cpp.o.requires:
.PHONY : CMakeFiles/query.dir/query.cpp.o.requires

CMakeFiles/query.dir/query.cpp.o.provides: CMakeFiles/query.dir/query.cpp.o.requires
	$(MAKE) -f CMakeFiles/query.dir/build.make CMakeFiles/query.dir/query.cpp.o.provides.build
.PHONY : CMakeFiles/query.dir/query.cpp.o.provides

CMakeFiles/query.dir/query.cpp.o.provides.build: CMakeFiles/query.dir/query.cpp.o

# Object files for target query
query_OBJECTS = \
"CMakeFiles/query.dir/query.cpp.o"

# External object files for target query
query_EXTERNAL_OBJECTS =

query: CMakeFiles/query.dir/query.cpp.o
query: CMakeFiles/query.dir/build.make
query: CMakeFiles/query.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable query"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/query.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/query.dir/build: query
.PHONY : CMakeFiles/query.dir/build

CMakeFiles/query.dir/requires: CMakeFiles/query.dir/query.cpp.o.requires
.PHONY : CMakeFiles/query.dir/requires

CMakeFiles/query.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/query.dir/cmake_clean.cmake
.PHONY : CMakeFiles/query.dir/clean

CMakeFiles/query.dir/depend:
	cd /home/ubuntu/edict/query && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ubuntu/edict/query /home/ubuntu/edict/query /home/ubuntu/edict/query /home/ubuntu/edict/query /home/ubuntu/edict/query/CMakeFiles/query.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/query.dir/depend
