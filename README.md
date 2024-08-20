Welcome to the Distributed File System project! This system is built using socket programming and is designed to handle file management across three different servers. Here's a breakdown of how everything works:

### Overview
This project sets up a distributed system with three main servers: `Smain`, `Spdf`, and `Stext`. The idea is that clients can upload, download, and manage files, but all they see is the `Smain` server. Behind the scenes, though, `Smain` cleverly organizes files based on their types:

- **Smain** is the main server where all client requests go. It stores `.c` files locally but sends `.pdf` files over to the `Spdf` server and `.txt` files to the `Stext` server. The client never sees this happening; they just interact with `Smain` as if all their files are stored there.
- **Spdf** is where all the `.pdf` files end up. `Smain` handles the transfer, so clients don't need to worry about it.
- **Stext** stores all the `.txt` files, again handled seamlessly by `Smain`.

### What Can You Do with the Client Program?
The client program (`client24s`) lets you perform several actions:

- **Upload files** with the `ufile` command. You can upload `.c`, `.pdf`, or `.txt` files to `Smain`. The system will either keep the file on `Smain` or send it to `Spdf` or `Stext`, depending on the file type.
- **Download files** with the `dfile` command. Whether the file is stored on `Smain`, `Spdf`, or `Stext`, you can download it easily using this command.
- **Delete files** with the `rmfile` command. Just like with downloads, you can delete any file stored on `Smain`, `Spdf`, or `Stext`.
- **Create tar files** with the `dtar` command. This allows you to bundle all files of a certain type (`.c`, `.pdf`, or `.txt`) into a tar file and download it.
- **Display file names** with the `display pathname` command. This lists all `.c`, `.pdf`, and `.txt` files in a specified directory, combining everything from `Smain`, `Spdf`, and `Stext` into one neat list. Only the filenames are sent to the client, not the actual files.

### Why This System Is Cool
- **It's Seamless**: The client interacts only with `Smain`, making everything feel simple and centralized, even though files are actually distributed across multiple servers.
- **Smart File Handling**: By automatically sorting files into the correct server based on their type, the system keeps everything organized and efficient.
- **Scalable Communication**: Everything is handled via sockets, making the system scalable and robust.

### What's in This Repo?
- `Smain.c`: The brain of the operation, handling client requests and file management.
- `Spdf.c`: Manages all `.pdf` files and communicates with `Smain`.
- `Stext.c`: Manages all `.txt` files and communicates with `Smain`.
- `client24s.c`: The client-side program that lets users interact with `Smain`.

### About the Author
This project was created by **Pramoth Jayaprakash**.
