/*
 * AlloyGMM.cpp
 *
 *  Created on: Sep 22, 2017
 *      Author: Blake Lucas
 */

// Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2008-2016 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------
//! \addtogroup gmm_full
//! @{
#include <AlloyGaussianMixture.h>
#include <AlloyOptimization.h>
namespace aly {
void SANITY_CHECK_GMM() {
	int G = 3;
	int D = 2;
	int S = 10000;
	std::vector<float> priors(G);
	std::vector<float2> sigmas(G);

	std::vector<float2> centers(G);

	std::cout << "Start Allocation" << std::endl;
	centers[0]= {1,2};
	centers[1]= {3,8};
	centers[2]= {6,6};

	sigmas[0] = float2(0.1f, 0.2f);
	sigmas[1] = float2(0.3f, 0.1f);
	sigmas[2] = float2(0.05f, 0.05f);

	//centers[3].data= {9,1};

	priors[0] = 0.3333f;
	priors[1] = 0.3333f;
	priors[2] = 0.3333f;
	//priors[3] = 0.25f;

	DenseMat<float> samples(D, S);
	std::cout << "Generate samples" << std::endl;
	std::vector<int> order(S);
	for (int i = 0; i < order.size(); i++) {
		order[i] = i;
	}
	Shuffle(order);
	for (int i = 0; i < S; i++) {
		int s = order[i];
		float thresh = RandomUniform(0.0f, 1.0f);
		float last = 0.0f;
		float curr = 0.0f;
		for (int k = 0; k < G; k++) {
			curr += priors[k];
			if (thresh >= last && thresh < curr) {
				float2 samp = float2(RandomGaussian(0.0f, 1.0f),
						RandomGaussian(0.0f, 1.0f)) * sigmas[k] + centers[k];
				Vec<float> sample(2);
				sample[0] = samp.x;
				sample[1] = samp.y;
				samples.setColumn(sample, i);
				break;
			}
			last = curr;
		}
	}
	GaussianMixture gmm;
	std::cout << "Start Learning " << samples.dimensions() << std::endl;
	gmm.learn(samples, G, 20, 20, 0.0001f);
}
GaussianMixture::GaussianMixture() {

}
void GaussianMixture::initializeParameters(const DenseMat<float>& X,
		float var_floor) {
	const int D = means.front().size();
	const int G = means.size();
	const int N = X.cols;
	if (N == 0) {
		return;
	}
	DenseMat<float> acc_means(D, G);
	DenseMat<float> acc_dcovs(D, G);
	acc_means.setZero();
	acc_dcovs.setZero();
	std::vector<int> sumMembers(G, 0);
	for (int i = 0; i < N; ++i) {
		VecMap<float> sample = X.getColumn(i);
		double min_dist = 1E30;
		int best_g = 0;
		for (int g = 0; g < G; ++g) {
			VecMap<float> mean = means[g];
			double dist = distanceSqr(sample, mean);

			if (dist < min_dist) {
				min_dist = dist;
				best_g = g;
			}
		}
		sumMembers[best_g]++;
		VecMap<float> acc_mean = acc_means.getColumn(best_g);
		VecMap<float> acc_dcov = acc_dcovs.getColumn(best_g);
		for (int d = 0; d < D; ++d) {
			float x_d = sample[d];
			acc_mean[d] += x_d;
			acc_dcov[d] += x_d * x_d;
		}
	}

	for (int g = 0; g < G; ++g) {
		VecMap<float> acc_mean = acc_means.getColumn(g);
		VecMap<float> acc_dcov = acc_dcovs.getColumn(g);
		int sumMember = sumMembers[g];
		Vec<float>& mean = means[g];
		DenseMat<float>& fcov = sigmas[g];
		fcov.setZero();
		for (int d = 0; d < D; ++d) {
			double tmp = acc_mean[d] / double(sumMember);
			mean[d] = (sumMember >= 1) ? tmp : (0);
			fcov[d][d] =
					(sumMember >= 2) ?
							float(
									(acc_dcov[d] / float(sumMember))
											- (tmp * tmp)) :
							float(var_floor);
		}
		priors[g] = sumMember / (float) N;
	}
//em_fix_params(var_floor);
}

void GaussianMixture::initializeMeans(const DenseMat<float>& X) {
	const int G = means.size();
	const int D = means.front().size();
	int N = X.cols;
// going through all of the samples can be extremely time consuming;
// instead, if there are enough samples, randomly choose samples with probability 0.1
	const int stride = 10;
	const bool use_sampling = ((N / int(stride * stride)) > G);
	const int step = (use_sampling) ? int(stride) : int(1);
	int start_index = RandomUniform(0, N - 1);
	means[0] = X.getColumn(start_index);
	double max_dist = 0.0;
	for (int g = 1; g < G; ++g) {
		int best_i = int(0);
		int start_i = int(0);
		max_dist = 0.0;
		for (int i = 0; i < N; i += step) {
			bool ignore_i = false;
			// find the average distance between sample i and the means so far
			size_t idx = i + RandomUniform(0, step - 1);
			VecMap<float> sample = X.getColumn(idx);
			double sum = 0.0;
			for (int h = 0; h < g; ++h) {
				double dist = distanceSqr(means[h], sample);
				if (dist == 0.0) {
					ignore_i = true;
					break;
				} else {
					sum += dist;
				}
			}
			if (!ignore_i && sum >= max_dist) {
				max_dist = sum;
				best_i = idx;
			}
		}
		// set the mean to the sample that is the furthest away from the means so far
		means[g] = X.getColumn(best_i);
	}
}
bool GaussianMixture::iterateKMeans(const DenseMat<float>& X, int max_iter) {
	const double ZERO_TOLERANCE = 1E-16;
	const int N = X.cols;
	const int D = means.front().size();
	const int G = means.size();
	std::vector<Vec<float>> acc_means(G, Vec<float>(D));
	std::vector<int> acc_hefts(G, 0);
	std::vector<int> last_indx(G, 0);
	std::vector<Vec<float>> new_means = means;
	for (int iter = 1; iter <= max_iter; ++iter) {
		acc_hefts.assign(acc_hefts.size(), 0);
		acc_means.assign(acc_means.size(), Vec<float>::zero(D));
		//Find closest cluster
		for (int i = 0; i < N; ++i) {
			VecMap<float> sample = X.getColumn(i);
			double min_dist = 1E30;
			int best_g = 0;
			for (int g = 0; g < G; ++g) {
				double dist = distanceSqr(means[g], sample);
				if (dist < min_dist) {
					min_dist = dist;
					best_g = g;
				}
			}
			acc_means[best_g] += sample;
			acc_hefts[best_g]++;
			last_indx[best_g] = i;
		}
		// generate new means
		for (int g = 0; g < G; ++g) {
			Vec<float>& acc_mean = acc_means[g];
			int acc_heft = acc_hefts[g];
			Vec<float>& new_mean = new_means[g];
			for (int d = 0; d < D; ++d) {
				new_mean[d] =
						(acc_heft >= 1) ?
								(acc_mean[d] / float(acc_heft)) : float(0);
			}
		}
		// heuristics to resurrect dead means in the even cluster centers collapse
		std::vector<int> dead_gs;
		for (int i = 0; i < acc_hefts.size(); i++) {
			if (acc_hefts[i] == 0) {
				dead_gs.push_back(i);
			}
			if (dead_gs.size() > 0) {
				std::vector<int> live_gs;
				for (int i = 0; i < acc_hefts.size(); i++) {
					if (acc_hefts[i] >= 2) {
						live_gs.push_back(i);
					}
				}
				std::sort(live_gs.begin(), live_gs.end(),
						[=](const int& a,const int& b) {return a>b;});

				if (live_gs.size() == 0) {
					return false;
				}
				int live_gs_count = 0;
				for (int dead_gs_count = 0;
						dead_gs_count < (int) dead_gs.size(); ++dead_gs_count) {
					const int dead_g_id = dead_gs[dead_gs_count];
					int proposed_i = 0;
					if (live_gs_count < live_gs.size()) {
						const int live_g_id = live_gs[live_gs_count];
						++live_gs_count;
						if (live_g_id == dead_g_id) {
							return false;
						}
						// recover by using a sample from a known good mean
						proposed_i = last_indx[live_g_id];
					} else {
						// recover by using a randomly selected sample (last resort)
						proposed_i = RandomUniform(0, N - 1);
					}
					if (proposed_i >= N) {
						return false;
					}
					new_means[dead_g_id] = X.getColumn(proposed_i);
				}
			}
			double rs_delta = 0;
			for (int g = 0; g < G; ++g) {
				rs_delta += distance(means[g], new_means[g]);
			}
			rs_delta /= G;
			if (rs_delta <= ZERO_TOLERANCE) {
				break;
			}
		}
		means = new_means;
	}
	return true;
}

bool GaussianMixture::learn(const DenseMat<float>& data, int G, int km_iter,
		int em_iter, float var_floor) {
	const float ZERO_TOLERANCE = 1E-16f;
	const float CONV_TOLERANCE = 1E-6f;
	int D = data.rows;
	int N = data.cols;
	DenseMat<float> W(N, G);
	DenseMat<float> U(D, D);
	DenseMat<float> Diag(D, D);
	DenseMat<float> Vt(D, D);
	means.resize(G, Vec<float>(D));
	priors.resize(G);
	sigmas.resize(G, DenseMat<float>(D, D));
	invSigmas.resize(G, DenseMat<float>(D, D));
	initializeMeans(data);
	if (km_iter > 0) {
		if (!iterateKMeans(data, km_iter)) {
			return false;
		}
	}
// initial fcovs
	initializeParameters(data, var_floor);

	std::vector<double> scaleFactors(G);
	double CORRECTION = std::pow(ALY_2_PI, data.rows * 0.5);
	double logl = 0;
	double lastlogl = 0;
	for (int iter = 0; iter < em_iter; iter++) {
		std::cout << "Iteration " << iter << std::endl;
		for (int k = 0; k < G; k++) {
			DenseMat<float>& M = sigmas[k];
			SVD(M, U, Diag, Vt);
			double det = 1;
			std::cout << k << ") " << means[k] << " ::" << priors[k];
			for (int k = 0; k < D; k++) {
				double d = Diag[k][k];
				std::cout<<" sigma["<<k<<"]="<<std::sqrt(d);
				if (std::abs(d) > ZERO_TOLERANCE) {
					det *= d;
					d = 1.0 / d;
				}
				Diag[k][k] = d;
			}
			std::cout<<std::endl;
			scaleFactors[k] = 1.0 / (CORRECTION * std::sqrt(det));
			invSigmas[k] = (U * Diag * Vt).transpose();

		}
		logl = 0;
		for (int n = 0; n < N; n++) {
			VecMap<float> sample = data.getColumn(n);
			double sum = 0;
			for (int k = 0; k < G; k++) {
				Vec<float>& mean = means[k];
				DenseMat<float> isig = invSigmas[k];
				double dgaus = std::exp(
						-0.5 * dot((sample - mean), isig * (sample - mean)))
						* scaleFactors[k] * priors[k];
				sum += dgaus;
				W[n][k] = dgaus;
			}
			logl += std::log(sum);
			for (int k = 0; k < G; k++) {
				W[n][k] /= sum;
			}
		}
		logl /= N;
		std::cout << "Log Likelihood= " << logl << std::endl;
		for (int k = 0; k < G; k++) {
			Vec<float>& mean = means[k];
			mean.setZero();
			double alpha = 0;
			for (int n = 0; n < N; n++) {
				VecMap<float> sample = data.getColumn(n);
				alpha += W[n][k];
				mean += W[n][k] * sample;
			}
			mean /= (float) alpha;
			DenseMat<float>& cov = sigmas[k];
			cov.setZero();
			for (int n = 0; n < N; n++) {
				VecMap<float> sample = data.getColumn(n);
				Vec<float> diff = (sample - mean);
				float w = W(n, k) / alpha;
				for (int ii = 0; ii < D; ii++) {
					for (int jj = 0; jj < D; jj++) {
						cov[ii][jj] += w * diff[ii] * diff[jj];
					}
				}
			}
			priors[k] = alpha / N;
		}
		if (std::abs(logl - lastlogl) < CONV_TOLERANCE) {
			break;
		}
		lastlogl = logl;
	}
	return true;
}
}
