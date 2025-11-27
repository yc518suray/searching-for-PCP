#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<array>
#include<vector>
#include<set>
#include<string>
#include<fstream>
#include<iostream>
#include"../lib/golay.h"
#include"../lib/equivalence.h"

using namespace std;

int main(int argc, char ** argv) {

    int ORDER = stoi(argv[1]);
    const int LEN = ORDER;

    printf("Filtering Equivalent Pairs...\n");

    char fname[100];
    sprintf(fname, "results/%d-unique-pairs-found", ORDER);
    FILE * out = fopen(fname, "w");

    //match every seq a with seq b, generate an equivalence class for this sequence

    set<GolayPair> classes;
    set<GolayPair> sequences;
    set<GolayPair> newset;

    char fname_a[200];
    char fname_b[200];
    // 檔名必須與 driver.sh 合併後的檔名一致
    sprintf(fname_a, "results/%d/%d-unique-filtered-a_final", ORDER, ORDER);
    sprintf(fname_b, "results/%d/%d-unique-filtered-b_final", ORDER, ORDER);

    std::ifstream pairs_a(fname_a);
    std::ifstream pairs_b(fname_b);

    if(!pairs_a.good() || !pairs_b.good()) {
        printf("Bad File: Could not open final A (%s) or B (%s) sequence file.\n", fname_a, fname_b);
        return 0;
    }

    std::string str_val;

    GolayPair seq;
    seq.a.resize(LEN);
    seq.b.resize(LEN);

    while(pairs_a.good() && pairs_b.good()) {

        bool read_success = true;

        // 讀取序列 A (LEN 個元素)
        for(int i = 0; i < LEN; i++) {
            if (!(pairs_a >> str_val)) {
                read_success = false;
                break;
            }
            try {
                seq.a[i] = stoi(str_val);
            } catch (const std::exception& e) {
                 // 處理無效數字 (例如檔案結尾的空白)
                read_success = false;
                break;
            }
        }

        // 如果 A 序列讀取失敗，則跳過 B 序列讀取
        if (!read_success) {
            goto end_of_files; // 跳到檔案結束標籤
        }

        // 讀取序列 B (LEN 個元素)
        for(int i = 0; i < LEN; i++) {
            if (!(pairs_b >> str_val)) {
                read_success = false;
                break;
            }
            try {
                seq.b[i] = stoi(str_val);
            } catch (const std::exception& e) {
                read_success = false;
                break;
            }
        }

        // 只有當 A 和 B 序列都完整讀取成功 (2 * LEN 個元素) 時才插入
        if (read_success) {
            sequences.insert(seq);
        }
    }
    end_of_files:;
    // --- 檔案讀取結束 ---

    printf("%llu sequences loaded.\n", sequences.size());

    printf("Constructing Generators\n");

    set<GolayPair> generators = constructGenerators(LEN);

    printf("Generating Equivalences\n");

    int count = 0;
/*
    set<GolayPair> equiv;

    auto prev = sequences.begin();
    auto it = sequences.begin();
    it++;
    while(it != sequences.end()) {

        if(equiv.find(*it) != equiv.end()) {
            sequences.erase(*it);

            it = prev;
        } else {
            set<GolayPair> classes = generateClassPairs(generators, *it);
            equiv.insert(classes.begin(), classes.end());
            count++;
            printf("%d classes generated\n", count);

            prev = it;
        }

        it++;
    }
*/

    for(auto it = sequences.begin(); it != sequences.end();) {
        set<GolayPair> classes = generateClassPairs(generators, *it);
        count++;
        printf("%d classes generated\n", count);

        if(classes.size() > sequences.size()) {

            for(auto iter = std::next(it, 1); iter != sequences.end();) {
                GolayPair current = *iter;
                iter++;
                if(classes.find(current) != classes.end()) {
                    sequences.erase(current);
                }
            }
            printf("Filtered. Size: %llu\n", sequences.size());
            it++;
        } else {
            set<GolayPair> newset;
            GolayPair base = *it;
            for(GolayPair seq : classes) {
                sequences.erase(seq);
            }
            sequences.insert(base);
            printf("Classwise filter. Size: %llu\n", sequences.size());
            it = sequences.find(base);
            it++;
        }
    }

    printf("%llu unique sequences found.\n", sequences.size());

    for(GolayPair seq : sequences) {
        for(int i = 0; i < LEN; i++) {
            fprintf(out, "%d ", seq.a[i]);
        }

        fprintf(out, " ");

        for(int i = 0; i < LEN; i++) {
            fprintf(out, "%d ", seq.b[i]);
        }

        fprintf(out, "\n");
    }

}
