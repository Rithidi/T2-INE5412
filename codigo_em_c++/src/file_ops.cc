#include "../include/file_ops.h"

int File_Ops::do_copyin(const char *filename, int inumber, INE5412_FS *fs) {
    FILE *file;
    int offset = 0, result, actual;
    char buffer[16384];

    file = fopen(filename, "r");
    if (!file) {
        std::cerr << "Couldn't open " << filename << std::endl;
        return 0;
    }

    while (1) {
        result = fread(buffer, 1, sizeof(buffer), file);
        if (result <= 0) break;
        if (result > 0) {
            actual = fs->fs_write(inumber, buffer, result, offset);
            if (actual < 0) {
                std::cerr << "ERROR: fs_write returned invalid result " << actual << std::endl;
                break;
            }
            offset += actual;
            if (actual != result) {
                std::cerr << "WARNING: fs_write only wrote " << actual << " bytes, not " << result << " bytes" << std::endl;
                break;
            }
        }
    }

    std::cout << offset << " bytes copied" << std::endl;
    fclose(file);
    return 1;
}

int File_Ops::do_copyout(int inumber, const char *filename, INE5412_FS *fs) {
    FILE *file;
    int offset = 0, result;
    char buffer[16384];

    file = fopen(filename, "w");
    if (!file) {
        std::cerr << "Couldn't open " << filename << std::endl;
        return 0;
    }

    while (1) {
        result = fs->fs_read(inumber, buffer, sizeof(buffer), offset);
        if (result <= 0) break;
        fwrite(buffer, 1, result, file);
        offset += result;
    }

    std::cout << offset << " bytes copied" << std::endl;
    fclose(file);
    return 1;
}
