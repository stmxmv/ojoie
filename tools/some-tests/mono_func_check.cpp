//
// Created by Aleudillonam on 5/12/2023.
//

//#include <mono/jit/jit.h>
//#include <mono/metadata/assembly.h>
//#include <mono/metadata/threads.h>
//#include <mono/utils/mono-logger.h>

#include <MonoLoader/MonoLoader.h>

//extern "C" {
//typedef int (*Fexport_test)(void);

extern "C" {
extern "C" __declspec(dllimport) int (*export_test)(void);
}


//}



int main() {

    export_test();



}