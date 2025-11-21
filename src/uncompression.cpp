#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<map>
#include<vector>
#include<set>
#include<array>
#include<time.h>
#include"fftw3.h"
#include"../lib/array.h"
#include"../lib/decomps.h"
#include"../lib/fourier.h"
#include"../lib/equivalence.h"
#include<tgmath.h>
#include<algorithm>
#include<fstream>
#include<string>
#include<omp.h>

double norm(fftw_complex dft) {
    return dft[0] * dft[0] + dft[1] * dft[1];
}

int main(int argc, char ** argv) {

    if (argc < 6) { 
        printf("Usage: uncompression <ORDER> <COMPRESS> <NEWCOMPRESS> <ProcNum> <InputFileName>\n");
        return -1;
    }
    // 重新定義參數讀取
    int ORDER = stoi(argv[1]);
    int COMPRESS = stoi(argv[2]);
    int NEWCOMPRESS = stoi(argv[3]);
    int procnum = stoi(argv[4]);
    std::string input_filename = argv[5]; // 讀取傳入的分割檔案名稱

    int LEN = ORDER / COMPRESS;

    printf("Proc %d: Uncompressing sequence of length %d, reading from %s\n", procnum, LEN, input_filename.c_str());

	//Su: FFTW multi-thread init
	fftw_init_threads();
	int num_threads = omp_get_max_threads();
	fftw_plan_with_nthreads(num_threads);

    fftw_complex *in, *out;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (ORDER / NEWCOMPRESS));
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (ORDER / NEWCOMPRESS));
    p = fftw_plan_dft_1d((ORDER) / NEWCOMPRESS, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    // 計算alphbaet, newalphabet, parts, partitions的部分移到平行化迴圈外
    std::set<int> alphabet;
    
    if(COMPRESS % 2 == 0) {
        for(int i = 0; i <= COMPRESS; i += 2) {
            alphabet.insert(i);
            alphabet.insert(-i);
        }
    } else {
        for(int i = 1; i <= COMPRESS; i += 2) {
            alphabet.insert(i);
            alphabet.insert(-i);
        }
    }

    std::set<int> newalphabet;

    if(NEWCOMPRESS % 2 == 0) {
        for(int i = 0; i <= NEWCOMPRESS; i += 2) {
            newalphabet.insert(i);
            newalphabet.insert(-i);
        }
    } else {
        for(int i = 1; i <= NEWCOMPRESS; i += 2) {
            newalphabet.insert(i);
            newalphabet.insert(-i);
        }
    }

    std::vector<std::vector<int>> parts = getCombinations(COMPRESS / NEWCOMPRESS, newalphabet);
    std::vector<std::vector<int>> partition;

    std::map<int, std::vector<std::vector<int>>> partitions;

    //generate all permutations of possible decompositions for each letter in the alphabet
    for(int letter : alphabet) {
        partition.clear();
        for(std::vector<int> part : parts) {
            int sum = 0;
            for(long unsigned int i = 0; i < part.size(); i++) {
                sum += part[i];
            }
            if(sum == letter) {
                do {
                    partition.push_back(part);
                } while(next_permutation(part.begin(), part.end()));
            }
        }
        partitions.insert(make_pair(letter, partition));
    }

    char fname[100];
    sprintf(fname, "results/%d-pairs-found", ORDER);
    std::ifstream file(input_filename);
    std::string line;
    std::string letter;

    if(!file) {
        printf("Error: Proc %d Bad file: %s\n", procnum, input_filename.c_str());
        return -1;
    }
    // 設定輸出檔案名稱並開啟
    char outafname[100], outbfname[100];
    sprintf(outafname, "results/%d/%d-unique-filtered-a_%d", ORDER, ORDER, procnum);
    sprintf(outbfname, "results/%d/%d-unique-filtered-b_%d", ORDER, ORDER, procnum);
    
    FILE * outa = fopen(outafname, "a"); 
    FILE * outb = fopen(outbfname, "a"); 

    if (!outa || !outb) {
        printf("Error: Cannot open output files.\n");
        // 在錯誤發生時也要清理 FFTW 資源
        fftw_free(in);
        fftw_free(out);
        fftw_destroy_plan(p);
        return -1;
    }
  
    long long i = 1;
    
    vector<int> origa;
    origa.resize(LEN);
    vector<int> origb;
    origb.resize(LEN);

    unsigned long long int count = 0;
    int curr = 0;
    vector<int> stack(LEN, 0); // 迴溯堆疊

    long long lines_processed = 0;
    while(file.good()) {

        // 讀取第一個序列 origa(原程式法稍作修改)
        i = 0;
        while(file.good() && i < LEN) {
            file >> letter;
            // 檢查是否為空字串或無效輸入
            if (letter.empty()) break; 
            try {
                origa[i] = stoi(letter);
                i++;
            } catch (const std::exception& e) {
                // 讀取錯誤或檔案結尾，中斷
                break;
            }
        }
        if (i < LEN) break; // 讀取失敗或檔案結束

        // 讀取第二個序列 origb(原程式法稍作修改)
        i = 0;
        while(file.good() && i < LEN) {
            file >> letter;
            if (letter.empty()) break;
            try {
                origb[i] = stoi(letter);
                i++;
            } catch (const std::exception& e) {
                break;
            }
        }
        if (i < LEN) break; 

        vector<int> seq;
        seq.resize(ORDER / NEWCOMPRESS);
        
        // 原程式碼shift original的部分
        //shift original sequence such that the element with the largest number of permutations is in the front
        set<int> seta;
        for(int element : origa) {
            seta.insert(element);
        }

        set<int> setb;
        for(int element : origb) {
            setb.insert(element);
        }
        int max = 0;
        int best = *seta.begin(); // 初始化為集合第一個元素
        for(int element : seta) {
            if(partitions.at(element).size() > max) {
                max = partitions.at(element).size();
                best = element;
            }
        }
        for(long unsigned int idx = 0; idx < origa.size(); idx++) {
            if(origa[idx] == best) {
                rotate(origa.begin(), origa.begin() + idx, origa.end());
                break;
            }
        }

        max = 0;
        best = *setb.begin(); // 初始化為集合第一個元素
        for(int element : setb) {
            if(partitions.at(element).size() > max) {
                max = partitions.at(element).size();
                best = element;
            }
        }
        for(long unsigned int idx = 0; idx < origb.size(); idx++) {
            if(origb[idx] == best) {
                rotate(origb.begin(), origb.begin() + idx, origb.end());
                break;
            }
        }
        set<vector<int>> perma;
        for(vector<int> perm : partitions.at(origa[0])) {
            set<vector<int>> equiv = generateUncompress(perm);
            perma.insert(*equiv.begin());
        }

        vector<vector<int>> newfirsta;
        for(vector<int> perm : perma) {
            newfirsta.push_back(perm);
        }

        set<vector<int>> permb;
        for(vector<int> perm : partitions.at(origb[0])) {
            set<vector<int>> equiv = generateUncompress(perm);
            permb.insert(*equiv.begin());
        }

        vector<vector<int>> newfirstb;
        for(vector<int> perm : permb) {
            newfirstb.push_back(perm);
        }
    //原程式
    //partitions = partitionsA;

    count = 0;
    curr = 0;
    std::fill(stack.begin(), stack.end(), 0);

    while(curr != -1) {

        while(curr != LEN - 1) {

            std::vector<int> permutation = partitions.at(origa[curr])[stack[curr]];

            if(curr == 0) {
                permutation = newfirsta[stack[curr]];
            }

            for(int i = 0; i < COMPRESS / NEWCOMPRESS; i++) {
                seq[curr + (LEN * i)] = permutation[i];
            }
            stack[curr]++;
            curr++;
        }

        //if curr is final element of original sequence, base case
        if(curr == LEN - 1) {

            for(std::vector<int> permutation : partitions.at(origa[curr])) {

                count++;

                for(int i = 0; i < COMPRESS / NEWCOMPRESS; i++) {
                    seq[curr + (LEN * i)] = permutation[i];
                }

                for(unsigned int i = 0; i < seq.size(); i++) {
                    in[i][0] = (double)seq[i];
                    in[i][1] = 0;
                } 

                fftw_execute(p);

                if(dftfilter(out, seq.size(), ORDER)) { 
                    for(unsigned int i = 0; i < seq.size() / 2; i++) {
                        fprintf(outa, "%d",    (int)rint(norm(out[i])));
                    }
                    fprintf(outa, " ");
                    for(int num : seq) {
                            fprintf(outa, "%d ", num);
                    }
                    fprintf(outa, "\n");
                }
            }
            
            curr--;

            while((unsigned int)stack[curr] == partitions.at(origa[curr]).size() || (curr == 0 && stack[curr] == newfirsta.size())) {
                stack[curr] = 0;
                curr--;
                if(curr == -1) {
                    curr = -1;
                    break;
                }
            }
            //printf("curr: %d, stack: %d\n", curr, stack[curr]);
        }
    }

    printf("%llu A sequences checked\n", count);
    count = 0;
    
    curr = 0;
    vector<int> stackb(LEN, 0);
    stack = stackb;

    //partitions = partitionsB;

    std::fill(stack.begin(), stack.end(), 0);
    
    while(curr != -1) {

        while(curr != LEN - 1) {
            std::vector<int> permutation = partitions.at(origb[curr])[stack[curr]];

            if(curr == 0) {
                permutation = newfirstb[stack[curr]];
            }

            for(int i = 0; i < COMPRESS / NEWCOMPRESS; i++) {
                seq[curr + (LEN * i)] = permutation[i];
            }
            stack[curr]++;
            curr++;
        }

        if(curr == LEN - 1) {

            for(std::vector<int> permutation : partitions.at(origb[curr])) {

                count++;

                for(int i = 0; i < COMPRESS / NEWCOMPRESS; i++) {
                    seq[curr + (LEN * i)] = permutation[i];
                }

                for(unsigned int i = 0; i < seq.size(); i++) {
                    in[i][0] = (double)seq[i];
                    in[i][1] = 0;
                } 

                fftw_execute(p);

                if(dftfilter(out, seq.size(), ORDER)) { 
                    for(unsigned int i = 0; i < seq.size() / 2; i++) {
                        fprintf(outb, "%d",    ORDER * 2 - (int)rint(norm(out[i])));
                    }
                    fprintf(outb, " ");
                    for(int num : seq) {
                            fprintf(outb, "%d ", num);
                    }
                    fprintf(outb, "\n");
                }
            }
            
            curr--;

            while((unsigned int)stack[curr] == partitions.at(origb[curr]).size() || (curr == 0 && stack[curr] == newfirstb.size())) {
                stack[curr] = 0;
                curr--;
                if(curr == -1) {
                    curr = -1;
                    break;
                }
            }
        }
    }
        lines_processed++;
    }
    
    printf("Proc %d: Finished processing %lld lines.\n", procnum, lines_processed);

    fftw_free(in);
    fftw_free(out);
    fftw_destroy_plan(p);

    fclose(outa);
    fclose(outb);
}
