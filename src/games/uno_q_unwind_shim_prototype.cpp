/* The following is some shim code that AI generated to fix the linker issue on
 * arduino uno q. Turns out that those issues are legitimate bugs and not a
 * misconfiguration on my part:
https://github.com/arduino/ArduinoCore-zephyr/pull/309
 * TODO: figure out how to get the latest version of this.
 *
//#define _GLIBCXX_USE_EXCEPTIONS 0
//
//extern "C" {
//
///* ===============================
//   ARM EABI unwind personality
//================================ */
//
// void __aeabi_unwind_cpp_pr0() {}
// void __aeabi_unwind_cpp_pr1() {}
//
///* ===============================
//   Core unwind entry points
//================================ */
//
// void _Unwind_Resume()
//{
//        while (1) {
//        }
//}
//
// void _Unwind_Complete() {}
//
// void __gnu_unwind_frame() {}
//
///* ===============================
//   Context accessors
//================================ */
//
// void *_Unwind_GetLanguageSpecificData() { return nullptr; }
// void *_Unwind_GetRegionStart() { return nullptr; }
// void *_Unwind_GetDataRelBase() { return nullptr; }
// void *_Unwind_GetTextRelBase() { return nullptr; }
//
///* ===============================
//   Register access
//================================ */
//
// int _Unwind_VRS_Get(...) { return 0; }
// int _Unwind_VRS_Set(...) { return 0; }
//
//// required by libstdc++
// void _Unwind_RaiseException()
//{
//         while (1) {
//         }
// }
//
// void _Unwind_DeleteException() {}
// }
