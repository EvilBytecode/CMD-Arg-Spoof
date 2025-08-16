
# CMD-Arg-Spoof

This project manipulates the Windows Process Environment Block (PEB) to fake the command line of a running process. It touches the lowlevel internals of Windows, particularly the `RTL_USER_PROCESS_PARAMETERS` structure, which stores process parameters like the image path and command line.

## What is `RTL_USER_PROCESS_PARAMETERS`?

`RTL_USER_PROCESS_PARAMETERS` is a structure embedded within the PEB that the Windows loader and runtime use to manage essential perprocess metadata. It is essentially the process's "bootstrapping info" and contains:

- `ImagePathName`: Full path to the executable as a `UNICODE_STRING`.
- `CommandLine`: The actual command line used to start the process, also as a `UNICODE_STRING`.
- `Environment`: Pointer to environment variables block.
- `CurrentDirectory`: The starting working directory of the process.
- Various windowing and startup flags, handles, and reserved internal fields.

References and deep dives:

- [Windows Internals Book](https://www.microsoftpressstore.com/store/windows-internals-part-1-system-architecture-processes-9780735684188)
- [NTDEV Wiki - PEB](https://www.nirsoft.net/kernel_struct/vista/PEB.html)
