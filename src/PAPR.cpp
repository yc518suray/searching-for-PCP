#include <iostream>
#include <vector>
#include <cmath>
#include <complex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <stdexcept>

#include"../lib/fourier.h"
#include"../lib/golay.h"
#include "fftw3.h"

double calculate_papr_linear(const std::vector<std::complex<double>>& time_domain_sequence) {
    if (time_domain_sequence.empty()) {
        return 0.0;
    }

    double max_power = 0.0;
    double total_power = 0.0;
    size_t N = time_domain_sequence.size();
    
    for (const auto& sample : time_domain_sequence) {
        //P_n = |x[n]|^2 = real^2 + imag^2
        double instantaneous_power = std::norm(sample); 
        
        max_power = std::max(max_power, instantaneous_power);
        total_power += instantaneous_power;
    }

    double average_power = total_power / N;
    
    if (average_power == 0.0) {
        return 0.0; 
    }
    
    // PAPR = P_peak / P_avg
    return max_power / average_power;
}


int main(int argc, char ** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ORDER> <LEN>" << std::endl;
        return 1;
    }

    int ORDER = std::stoi(argv[1]);
    int LEN = std::stoi(argv[2]);
    int N = LEN; // fft length = sequence length

    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    
    fftw_plan p_ifft = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    char fname[100];
    std::sprintf(fname, "results/%d-unique-pairs-found", ORDER);

    std::ifstream infile(fname);
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << fname << std::endl;
        fftw_destroy_plan(p_ifft);
        fftw_free(in);
        fftw_free(out);
        return 1;
    }

    std::string line;
    int pair_count = 0;
    
    std::cout << "\n--- Golay Pair PAPR  ---\n";
    std::cout << "Pair | PAPR (A) | PAPR (B)\n";
    std::cout << "-----------------------------------\n";
    std::cout << std::fixed << std::setprecision(6); // increase output precision

    while (std::getline(infile, line)) {
       pair_count++;

        // use stringstream to parse the line
        std::stringstream ss(line);
        int value;
        std::vector<int> full_sequence;

        while (ss >> value) {
            full_sequence.push_back(value);
        }

        // 2. check length
        if (full_sequence.size() != 2 * N) {
            // error handling
            continue;
        }

        // 3. seperate A and B
        std::vector<int> seqa(full_sequence.begin(), full_sequence.begin() + N);
        std::vector<int> seqb(full_sequence.begin() + N, full_sequence.end());

        fftw_complex *xA_fftw = dft(seqa, in, out, p_ifft); 
        
        std::vector<std::complex<double>> xA_time_domain;
        for(int i = 0; i < N; ++i) {
            // IFFT output devided by N normalization
            xA_time_domain.emplace_back(xA_fftw[i][0] / N, xA_fftw[i][1] / N); 
        }
        double paprA_linear = calculate_papr_linear(xA_time_domain);

        fftw_complex *xB_fftw = dft(seqb, in, out, p_ifft);
        
        std::vector<std::complex<double>> xB_time_domain;
        for(int i = 0; i < N; ++i) {
            xB_time_domain.emplace_back(xB_fftw[i][0] / N, xB_fftw[i][1] / N); 
        }
        double paprB_linear = calculate_papr_linear(xB_time_domain);

        // output results
        std::cout << std::setw(4) << pair_count << " | "
                  << std::setw(10) << paprA_linear << " | "
                  << std::setw(10) << paprB_linear << std::endl;
    }

    //clean fftw resources
    fftw_destroy_plan(p_ifft);
    fftw_free(in);
    fftw_free(out);

    std::cout << "\n done,excuted " << pair_count << " pairs。" << std::endl;
    
    return 0;
}

