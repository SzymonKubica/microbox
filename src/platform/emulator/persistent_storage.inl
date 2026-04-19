#ifdef EMULATOR
#include <fstream>
#include <iostream>

template <typename T> T &PersistentStorage::get(int offset, T &t)
{
        std::ifstream ifs("persistent_storage.bin", std::ios::binary);

        if (!ifs) {
                std::cerr << "Error opening file for reading." << std::endl;
                return t; // Return the original object if file opening fails
        }
        ifs.seekg(0, std::ios::end);
        std::streamsize file_size = ifs.tellg();

        if (file_size < offset) {
                std::cerr << "File too small: " << file_size
                          << ", returning default" << std::endl;
                return t;
        }

        ifs.seekg(offset);
        ifs.read(reinterpret_cast<char *>(&t), sizeof(T));
        ifs.close();

        return t;
}

template <typename T> const T &PersistentStorage::put(int offset, const T &t)
{
        std::fstream ofs;
        // We need to specify both in and out to avoid having the truncate setting
        // clear the file before writing to it.
        ofs = std::fstream("persistent_storage.bin",
                         std::ios::in | std::ios::out | std::ios::binary);

        if (!ofs) {
                std::cerr << "Error opening file for writing in append mode. falling back to write only mode" << std::endl;
                ofs = std::fstream("persistent_storage.bin",
                         std::ios::out | std::ios::binary);
        }

        if (!ofs) {
                std::cerr << "Error opening file for writing." << std::endl;
                return t; // Return the original object if file opening fails
        }

        ofs.seekp(0, std::ios::end);
        std::streamsize file_size_before = ofs.tellp();
        std::cout << "File size before writing: " << file_size_before
                  << std::endl;

        std::cout << "Writing to persistent storage at offset: " << offset
                  << std::endl;
        ofs.seekp(offset, std::ios::beg);
        ofs.write(reinterpret_cast<const char *>(&t), sizeof(T));

        ofs.seekp(0, std::ios::end);
        std::streamsize file_size = ofs.tellp();
        std::cout << "File size after writing: " << file_size << std::endl;
        ofs.close();
        return t;
}
#endif
