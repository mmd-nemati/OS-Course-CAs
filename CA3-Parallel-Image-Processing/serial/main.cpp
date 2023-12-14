#include <fstream>
#include <iostream>
#include <algorithm>

#define EQ(x, y) abs(x - y) <= 0.00001f


constexpr char OUTPUT_FILE[] = "output.bmp";
constexpr int MAX_RGB_VALUE = 255;
constexpr int MIN_RGB_VALUE = 0;
constexpr int GAUSSIAN_BLUR_KERNEL[3][3] = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
constexpr double NORMALIZE_FACTOR = 1.0/16.0;

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER {
    WORD bf_type;
    DWORD bf_size;
    WORD bf_reserved1;
    WORD bf_reserved2;
    DWORD bf_off_bits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD bi_size;
    LONG bi_width;
    LONG bi_height;
    WORD bi_planes;
    WORD bi_bit_count;
    DWORD bi_compression;
    DWORD bi_size_image;
    LONG bi_x_pels_per_meter;
    LONG bi_y_pels_per_meter;
    DWORD bi_clr_used;
    DWORD bi_clr_important;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
#pragma pack(pop)

struct Pixel {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};


Pixel **photo;
char *file_buffer;
int buffer_size;
int rows;
int cols;


bool fill_and_allocate(char*& buffer, const char* file_name, int& rows, int& cols, int& buffer_size) {
    std::ifstream file(file_name);
    if (!file) {
        std::cout << "File " << file_name << " doesn't exist!" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->bi_height;
    cols = info_header->bi_width;
    buffer_size = file_header->bf_size;
    return true;
}

void get_pixels_from_bmp24() {
    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {
            for (int k = 0; k < 3; k++) {
                switch (k) {
                case 0:
                    photo[i][j].red = file_buffer[buffer_size - count];
                    break;
                case 1:
                    photo[i][j].green = file_buffer[buffer_size - count];
                    break;
                case 2:
                    photo[i][j].blue = file_buffer[buffer_size - count];
                    break;
                }
                count++;
            }
        }
    }
}

void write_out_bmp24() {
    std::ofstream write(OUTPUT_FILE);
    if (!write) {
        std::cout << "Failed to write " << OUTPUT_FILE << std::endl;
        return;
    }

    int count = 1;
    int extra = cols % 4;
    for (int i = 0; i < rows; i++) {
        count += extra;
        for (int j = cols - 1; j >= 0; j--) {
            for (int k = 0; k < 3; k++) {
                switch (k) {
                case 0:
                    file_buffer[buffer_size - count] = photo[i][j].red;
                    break;
                case 1:
                    file_buffer[buffer_size - count] = photo[i][j].green;
                    break;
                case 2:
                    file_buffer[buffer_size - count] = photo[i][j].blue;
                    break;
                }
                count++;
            }
        }
    }
    write.write(file_buffer, buffer_size);
}

Pixel** make_photo() {
    Pixel** photo = new Pixel*[rows];
    for (int i = 0; i < rows; i++)
        photo[i] = new Pixel[cols];
    return photo;
}

void alloc_photo() {
    photo = make_photo();
}

void init(char* input_file_name) {
    if (!fill_and_allocate(file_buffer, input_file_name, rows, cols, buffer_size)) {
        std::cout << "ERROR: reading input file failed" << std::endl;
        exit(1);
    }
    alloc_photo();
}

void flip_photo_filter() {
    for (int i = 0; i < rows / 2; i++)
        for (int j = 0; j < cols; j++)
            std::swap(photo[i][j], photo[rows - i - 1][j]);
}


void blur_photo_filter() {
    Pixel** prev = make_photo();
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            prev[i][j] = photo[i][j];

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int tmp_red = 0;
            int tmp_green = 0;
            int tmp_blue = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int x_neighbor = i + dx;
                    int y_neighbor = j + dy;
                    if (x_neighbor >= 0 && x_neighbor < rows && y_neighbor >= 0 && y_neighbor < cols) {
                        tmp_red += int(prev[x_neighbor][y_neighbor].red) * GAUSSIAN_BLUR_KERNEL[dx + 1][dy + 1] * NORMALIZE_FACTOR;
                        tmp_green += int(prev[x_neighbor][y_neighbor].green) * GAUSSIAN_BLUR_KERNEL[dx + 1][dy + 1] * NORMALIZE_FACTOR;
                        tmp_blue += int(prev[x_neighbor][y_neighbor].blue) * GAUSSIAN_BLUR_KERNEL[dx + 1][dy + 1] * NORMALIZE_FACTOR;
                    }
                }
            }
            photo[i][j].red = std::clamp(tmp_red, MIN_RGB_VALUE, MAX_RGB_VALUE);
            photo[i][j].green = std::clamp(tmp_green, MIN_RGB_VALUE, MAX_RGB_VALUE);
            photo[i][j].blue = std::clamp(tmp_blue, MIN_RGB_VALUE, MAX_RGB_VALUE);
        }
    }
}

void purple_haze_filter() {
    Pixel** prev = make_photo();
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            prev[i][j] = photo[i][j];

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) {
            int tmp_red = double(prev[i][j].red) * 0.5 + double(prev[i][j].green) * 0.3 + double(prev[i][j].blue) * 0.5;
            int tmp_green = double(prev[i][j].red) * 0.16 + double(prev[i][j].green) * 0.5 + double(prev[i][j].blue) * 0.16;
            int tmp_blue = double(prev[i][j].red) * 0.6 + double(prev[i][j].green) * 0.2 + double(prev[i][j].blue) * 0.8;

            photo[i][j].red = std::clamp(tmp_red, MIN_RGB_VALUE, MAX_RGB_VALUE);
            photo[i][j].green = std::clamp(tmp_green, MIN_RGB_VALUE, MAX_RGB_VALUE);
            photo[i][j].blue = std::clamp(tmp_blue, MIN_RGB_VALUE, MAX_RGB_VALUE);
        }

}

void draw_lines_filter() {
    int mid_row = rows / 2;
    int mid_col = cols / 2;
    
    float slope1 = -(float)(mid_row) / (float)(mid_col);
    float slope2 = -(float)(rows) / (float)(cols);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            if (EQ(j * slope1 + mid_row, float(i)) || EQ(j * slope1 + mid_row + rows, float(i)) || EQ(j * slope2 + rows, float(i))) {
                photo[i][j].red = MAX_RGB_VALUE;
                photo[i][j].green = MAX_RGB_VALUE;
                photo[i][j].blue = MAX_RGB_VALUE;
            }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <input_file_name>" << std::endl;
        return 1;
    }
    init(argv[1]);

    get_pixels_from_bmp24();
    // apply filters
    flip_photo_filter();
    blur_photo_filter();
    purple_haze_filter();
    draw_lines_filter();
    // end filters0
    write_out_bmp24();

    return 0;
}
