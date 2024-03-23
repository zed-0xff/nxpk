#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <string>
#include <map>

#define ROL32(x, n) x = (((x) << (n)) | ((x) >> (32 - (n))))
#define ROR32(x, n) x = (((x) >> (n)) | ((x) << (32 - (n))))

uint32_t eax, edx;
int cf;
int ncoll = 0, nold = 0;
bool test_only = false;

void mul(uint32_t x) {
    uint64_t result = (uint64_t)eax * x;
    eax = result;
    edx = result >> 32;
    cf = (edx == 0) ? 0 : 1;
}

void adc(uint32_t& a, uint32_t b){
    uint64_t result = (uint64_t)a + b + cf;
    a = result;
    cf = (result >> 32) == 0 ? 0 : 1;
}

void add(uint32_t& a, uint32_t b){
    uint64_t result = (uint64_t)a + b;
    a = result;
    cf = (result >> 32) == 0 ? 0 : 1;
}

uint32_t npk_hash2(const char* s) {
    uint32_t ebp = 0xF4FA8928;
    uint32_t ebx = 0;
    uint32_t ecx = 0x37A8470E;
    uint32_t edi = 0x7758B42B;
    eax = edx = 0;
    cf = 0;

    while(1){
        ebx = 0x267B0B11;
        ROL32(ebp, 1);
        ebx ^= ebp;
        eax = (eax & 0xffffff00) | *s++;
        if ((eax & 0xFF) == 0 ){
            goto zerobyte;
        }
        ROR32(eax, 8);
        eax = (eax & 0xffffff00) | *s++;
        ROL32(eax, 8);
        if ((eax & 0xFF00) == 0) {
            eax &= 0xFF;
            break;
        }
        ROR32(eax, 16);
        eax = (eax & 0xffffff00) | *s++;
        ROL32(eax, 16);
        if ((eax & 0xFF0000) == 0) {
            eax &= 0xFFFF;
            break;
        }
        ROR32(eax, 0x18);
        eax = (eax & 0xffffff00) | *s++;
        ROL32(eax, 0x18);
        if ((eax & 0xFF000000) == 0) {
            break;
        }
        ecx ^= eax;
        edx = ebx;
        edi ^= eax;
        edx += edi;
        edx |= 0x02040801;
        edx &= 0xBFEF7FDF;
        eax = ecx;
        mul(edx);
        adc(eax, edx);
        edx = ebx;
        adc(eax, 0);
        edx += ecx;
        edx |= 0x0804021;
        edx &= 0x7DFEFBFF;
        ecx = eax;
        eax = edi;
        mul(edx);
        add(edx, edx);
        adc(eax, edx);
        if (cf) {
            eax += 2;
        }
        edi = eax;
    }

    ecx ^= eax;
    edx = ebx;
    edi ^= eax;
    edx += edi;
    edx |= 0x02040801;
    edx &= 0xBFEF7FDF;
    eax = ecx;
    mul(edx);
    adc(eax, edx);
    edx = ebx;
    adc(eax, 0);
    edx += ecx;
    edx |= 0x00804021;
    edx &= 0x7DFEFBFF;
    ecx = eax;
    eax = edi;
    mul(edx);
    add(edx, edx);
    adc(eax, edx);
    if (cf) {
        eax += 2;
    }
    edi = eax;
    ebx = 0x267B0B11;
    ROL32(ebp, 1);
    ebx ^= ebp;

zerobyte:
    ecx ^= 0x9BE74448;
    edx = ebx;
    edi ^= 0x9BE74448;
    edx += edi;
    edx |= 0x02040801;
    edx &= 0xBFEF7FDF;
    eax = ecx;
    mul(edx);
    adc(eax, edx);
    edx = ebx;
    adc(eax, 0);
    edx += ecx;
    edx |= 0x00804021;
    edx &= 0x7DFEFBFF;
    ecx = eax;
    eax = edi;
    mul(edx);
    add(edx, edx);
    adc(eax, edx);
    if (cf) {
        eax += 2;
    }
    edi = eax;
    ebx = 0x267B0B11;
    ROL32(ebp, 1);
    ebx ^= ebp;
    ecx ^= 0x66F42C48;
    edx = ebx;
    edi ^= 0x66F42C48;
    edx += edi;
    edx |= 0x02040801;
    edx &= 0xBFEF7FDF;
    eax = ecx;
    mul(edx);
    adc(eax, edx);
    edx = ebx;
    adc(eax, 0);
    edx += ecx;
    edx |= 0x00804021;
    edx &= 0x7DFEFBFF;
    ecx = eax;
    eax = edi;
    mul(edx);
    add(edx, edx);
    adc(eax, edx);
    if (cf) {
        eax += 2;
    }
    edx = ecx;
    eax ^= ecx;

    return eax;
}

void test(const char* data){
    uint32_t hash = npk_hash2(data);
    printf("%08x %s\n", hash, data);
}

const char* keys_fname = nullptr;
std::map<uint32_t, std::string> all_keys;
char line[0x10000];

void load_keys(){
    FILE* f;

    f = fopen(keys_fname, "r");
    if( f == nullptr ){
        fprintf(stderr, "Failed to open file: %s\n", keys_fname);
        exit(1);
    }

    int nempty = 0;
    while( fgets(line, sizeof(line), f) ){
        if( line[0] == '#' || line[0] == '\n' ){
            continue;
        }
        if( line[8] != ' ' && line[8] != '\n' ){
            fprintf(stderr, "Invalid line: %s", line);
            exit(1);
        }
        uint32_t key = strtoul(line, nullptr, 16);
        std::string value = line + 9;
        if( value.back() == '\n' )
            value.pop_back();

        all_keys[key] = value;
        if( value.empty() ){
            nempty++;
        }
    }
    fclose(f);
    printf("[*] loaded %ld hashes from %s, unknown: %d\n", all_keys.size(), keys_fname, nempty);
}

void save_keys(){
    FILE* f;

    f = fopen(keys_fname, "w");
    if( f == nullptr ){
        fprintf(stderr, "Failed to open file: %s\n", keys_fname);
        exit(1);
    }

    for( auto it = all_keys.begin(); it != all_keys.end(); it++ ){
        fprintf(f, "%08x %s\n", it->first, it->second.c_str());
    }
    fclose(f);
}

bool check_string(const char* line){
    uint32_t hash = npk_hash2(line);
    auto it = all_keys.find(hash);
    if( it == all_keys.end() )
        return false;

    if( !it->second.empty() ){
        if( it->second == line ){
            //printf("[-] %08x: already present: \"%s\"\n", hash, line);
            nold++;
        } else {
            ncoll++;
            printf("[?] %08x: collision! existing \"%s\" vs new \"%s\"\n", hash, it->second.c_str(), line);
        }
        return false;
    }
    printf("[+] %08x: %s\n", hash, line);
    all_keys[hash] = line;
    return true;
}

void process_wordlist(const char* wordlist_fname){
    int nchecked = 0;

    if( all_keys.empty() )
        load_keys();

    FILE* f;

    if( strcmp(wordlist_fname, "-") == 0 ){
        printf("[*] processing STDIN ..\n");
        f = stdin;
    } else {
        printf("[*] processing %s ..\n", wordlist_fname);
        f = fopen(wordlist_fname, "r");
        if( f == nullptr ){
            fprintf(stderr, "Failed to open file: %s\n", wordlist_fname);
            exit(1);
        }
    }

    int nnew = 0;
    while( fgets(line, sizeof(line), f) ){
        if( line[0] == '\n' ){
            continue;
        }
        if( line[strlen(line) - 1] == '\n' ){
            line[strlen(line) - 1] = 0;
        }
        if( line[strlen(line) - 1] == '\r' ){
            line[strlen(line) - 1] = 0;
        }
        nchecked++;
        if( check_string(line) )
            nnew++;
    }

    if( f != stdin )
        fclose(f);

    printf("[=] checked %d keys: %d new, %d old, %d collisions\n\n", nchecked, nnew, nold, ncoll);
    if( ncoll ){
    } else {
        printf("[=] found %d new keys\n\n", nnew);
    }

    if( nnew > 0 && !test_only )
        save_keys();
}

void process_strings(char** begin, char** end){
    if( all_keys.empty() )
        load_keys();

    int nnew = 0;
    for( char** it = begin; it != end; it++ ){
        if( check_string(*it) )
            nnew++;
    }

    if( nnew > 0 && !test_only )
        save_keys();
}

int main(int argc, char* argv[]) {
    if( argc < 4 ){
        printf("Usage:\n");
        printf("    keytool <keys_fname> -f <wordlist1.txt> .. [wordlistN.txt]\n");
        printf("    keytool <keys_fname> -tf <wordlist1.txt> .. [wordlistN.txt]\n");
        printf("    keytool <keys_fname> -s <string1> .. [stringN]\n");
        printf("    keytool <keys_fname> -h <string1>\n");
        return argc == 1 ? 0 : 1;
    }

    keys_fname = argv[1];

    if( strcmp(argv[2], "-tf") == 0 ){
        test_only = true;
        for( int i = 3; i < argc; i++ ){
            process_wordlist(argv[i]);
        }
        return 0;
    }

    if( strcmp(argv[2], "-f") == 0 ){
        for( int i = 3; i < argc; i++ ){
            process_wordlist(argv[i]);
        }
        return 0;
    }

    if( strcmp(argv[2], "-s") == 0 ){
        process_strings(&argv[3], &argv[argc]);
        return 0;
    }

    if( strcmp(argv[2], "-h") == 0 ){
        uint32_t hash = npk_hash2(argv[3]);
        printf("%08x %s\n", hash, argv[3]);
        return 0;
    }

    fprintf(stderr, "Invalid option: %s\n", argv[2]);
    return 1;
}
