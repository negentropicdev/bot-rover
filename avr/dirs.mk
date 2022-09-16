#These parameters define the source / output structure and can be changed
#to reflect a different layout.

#INCLUDE_DIR specifies where to find header files outside of the source tree.
# This can be commented out if a common include tree isn't being set up for
# easy building of other projects using this source tree.
INCLUDE_DIR:=$(projroot)/include

#LIB_DIR specifies where to place archive files (static libs)
LIB_DIR:=$(projroot)/lib

#BIN_DIR specifies where to place compiled executables.
BIN_DIR:=$(projroot)/bin

#BUILD_DIR specifies the folder used for intermediate build files.
BUILD_DIR:=$(projroot)/build

#SRC_DIR is the root of the source code for the projects
#This is used to strip out that relative path for organizing files in the BUILD_DIR
SRC_DIR:=$(projroot)/src