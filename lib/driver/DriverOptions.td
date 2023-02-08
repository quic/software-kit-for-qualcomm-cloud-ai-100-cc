// Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

include "llvm/Option/OptParser.td"

// Help
def help : Flag<["-", "--"], "help">, HelpText<"Display help">;
def h : Flag<["-"], "h">, Alias<help>;

// QAIC Application
def Program_Config : Separate<["-", "--"], "qaic-program-config">, HelpText<"QAIC program configuration file">;

// Inputs / Outputs
def o : JoinedOrSeparate<["-"], "o">, HelpText<"Write output to <file>">, MetaVarName<"<file>">;

// Source Compilation
def c : Flag<["-"], "c">, HelpText<"Only run preprocess, compile, and assemble steps">;
def S : Flag<["-"], "S">, HelpText<"Only run preprocess, and compile, steps">;
def E : Flag<["-"], "E">, HelpText<"Only run preprocess step">;

def I : JoinedOrSeparate<["-"], "I">, MetaVarName<"<dir>">, HelpText<"Add directory to include search path">;
def isystem : JoinedOrSeparate<["-"], "isystem">, HelpText<"Add directory to SYSTEM include search path">, MetaVarName<"<directory>">;

def D : JoinedOrSeparate<["-"], "D">, HelpText<"Define a preprocessor macro value">;
def U : JoinedOrSeparate<["-"], "U">, HelpText<"Undefine a preprocessor macro value">;

def g : Flag<["-"], "g">, HelpText<"Generate source-level debug information">;
def g0 : Flag<["-"], "g0">, HelpText<"Disable source-level debug information">;

def stdcxx : Joined<["-", "--"], "std=">, HelpText<"Specifies the C++ standard to compile for.">;

def save_temps : Flag<["-"], "save-temps">, HelpText<"Save temporary outputs from compilation">;

// Warnings
def W_Joined : Joined<["-"], "W">, MetaVarName<"<warning>">, HelpText<"Enable the specified warning">;

// Optimize flags
def O0 : Flag<["-"], "O0">;
def O : Flag<["-"], "O">, Alias<O0>;
def O1 : Flag<["-"], "O1">;
def O2 : Flag<["-"], "O2">;
def O3 : Flag<["-"], "O3">;

// Linker
def Wl_comma : CommaJoined<["-"], "Wl,">, HelpText<"Pass the arguments in <arg> to the linker">;
def L : JoinedOrSeparate<["-"], "L">, MetaVarName<"<dir>">, HelpText<"Add directory to linker search path">;
def NoDefaultLibPaths : Flag<["-"], "no-defaultlib-paths">, HelpText<"Disables any default paths the linker will search.">;
def u : JoinedOrSeparate<["-"], "u">, MetaVarName<"<symbol>">, HelpText<"Specify undefined symbol to the linker.">;
def l : Joined<["-"],"l">, HelpText<"Link the named library">;

// By default we don't want to link the standard library but some apps currently do so.
// We either should support linking with libc proper or add the necessary functionality to the QAIC runtime
def WithStdLibraries : Flag<["-"], "with-stdlib">, HelpText<"Enables linking in the standard library.">;

// Diagnostics
def Debug : Separate<["-", "--"], "debug">, HelpText<"Print out debug output from the tool">;
