# PthreadKeyDetect
PthreadKeyDetect is a tool to detect the available number of Android **pthread_key** and hook pthread_key-related functions.

## Description

In Android devices, every `pthread_key_create` function invoked will cost one `pthread_key_t` key, there is a limit of `PTHREAD_KEYS_MAX(128 or 64)` keys per process. The compiler will automatically use ELF TLS (minSdkVersion >= 29, see [Android linker changes for NDK developers](https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md)), but many Apps use minSdkVersion <29. So, there may be unpredictable crashes if your App costs more than the `PTHREAD_KEYS_MAX` keys in one process. The most common keywords in the crash stack are: `pthread_once`、`emutls_get_address`、`cxa_get_globals`、`emutls_init`.

Therefore, this tool is to help troubleshoot the problem of the number of pthread_key used.

The main features are:

- Get the maximum pthread_key_t count(`PTHREAD_KEYS_MAX`) that the device supports.
- Get the maximum pthread_key_t count that the app's current process can be created in the current status.
- Hook `pthread_key_create` and `pthread_key_delete` functions call,  **Non-invasive.** Hook It is based on [Tencent/matrix](https://github.com/Tencent/matrix).



## Getting started

The library only has one class `PthreadKeyDetect`

- `hook()`hook native 'pthread_key_delete' and 'pthread_key_delete' functions, you can find who use pthread_key_t.
- `available()` the maximum pthread_can_t count that the app's current process can be created in the current status.
- `max()`returns the maximum pthread_key_t count that the device supports creating.
- `useUp()` use up all pthread_key_t, can test extreme cases, how your app works



### NOTE

This library only run in the test environment, don't use it in the product environment, there may be unexpected errors.
