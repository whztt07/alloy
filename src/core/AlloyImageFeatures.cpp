

#include "AlloyImageFeatures.h"
#include "AlloyImageProcessing.h"
namespace aly {
	namespace daisy {
		const float Daisy::sigma_0 = 1.0f;
		const float Daisy::sigma_1 = std::sqrt(2.0f);
		const float Daisy::sigma_2 = 8.0f;
		const float Daisy::sigma_step = (float)std::pow(2, 1.0f / 2);
		const int Daisy::scale_st = int((std::log(sigma_1 / sigma_0)) / (float)std::log(sigma_step));
		Daisy::Daisy(int descSize):descriptorSize(descSize) {
		}
		void Daisy::evaluate(const ImageRGBAf& image,float _descriptorRadius,int _radiusBins,int _angleBins,int _histogramBins) {
			histogramBins = _histogramBins;
			angleBins = _angleBins;
			radiusBins = _radiusBins;
			descriptorRadius = _descriptorRadius;
			numberOfGridPoints = _angleBins * _radiusBins + 1; // +1 is for center pixel
			descriptorSize = numberOfGridPoints * _histogramBins;
			for (int i = 0; i<360; i++){
				orientatioShift[i] = i / 360.0f * _histogramBins;
			}
			layerSize = image.width*image.height;
			cubeSize = layerSize*_histogramBins;
			computeCubeSigmas();
			computeGridPoints();
		}
		int Daisy::quantizeRadius(float rad) {
			if (rad <= sigmas[0]) return 0;
			if (rad >= sigmas[sigmas.size() - 1]) return (int)(sigmas.size() - 1);
			float dist;
			float mindist =std::numeric_limits<float>::max();
			int mini = 0;
			for (int c = 0; c<sigmas.size(); c++) {
				dist = std::abs(sigmas[c] - rad);
				if (dist < mindist) {
					mindist = dist;
					mini = c;
				}
			}
			return mini;
		}
		void Daisy::computeScales() {
			float sigma = std::pow(sigma_step, scale_st)*sigma_0;
			Image1f sim;
			Image1f next_sim;
			Image1f max_dog(image.width, image.height);
			scaleMap.resize(image.width, image.height);
			scaleMap.setZero();
			max_dog.setZero();
			Smooth<5, 5>(image, sim, sigma, sigma);
			float sigma_prev= sigma_0;
			float sigma_new;
			float sigma_inc;
			std::cout << "Sigma " << sigma_prev << std::endl;
			for (int i = 0; i<scale_en; i++)
			{
				std::cout << "Sigma new " << sigma_new << std::endl;
				sigma_new = std::pow(sigma_step, scale_st + i) * sigma_0;
				sigma_inc = std::sqrt(sigma_new*sigma_new - sigma_prev*sigma_prev);
				sigma_prev = sigma_new;
				Smooth<5, 5>(sim, next_sim, sigma_inc, sigma_inc);
				for (int p = 0; p<(int)image.size(); p++)
				{
					float dog = fabs(next_sim[p] - sim[p]);
					if (dog > max_dog[p])
					{
						max_dog[p] = float1(dog);
						scaleMap[p] = float1((float)i);
					}
				}
				sim = next_sim;
			}
			//smooth scaling map
			Smooth<21,21>(scaleMap, sim, 10,10);
			scaleMap = sim;
			for (int q = 0; q<(int)sim.size(); q++)
			{
				scaleMap[q] = std::floor(scaleMap[q]+0.5f);
			}
		}

		void Daisy::layeredGradient(const Image1f& image,std::vector<Image1f>& layers, int layer_no)
		{
			Image1f bdata;
			Image1f dx, dy;
			Gradient<5,5>(image, dx, dy);
			layers.resize(layer_no);
			for (int l = 0; l<layer_no; l++){
				Image1f& layer_l = layers[l];
				layer_l.resize(image.width, image.height);
				layer_l.setZero();
				float angle = 2.0f * l * ALY_PI / layer_no;
				float cosa = std::cos(angle);
				float sina = std::sin(angle);
				for (int index = 0; index<image.size(); index++){
					float value = cosa * dx[index] + sina * dy[index];
					if (value > 0) {
						layer_l[index] = value;
					}
					else {
						layer_l[index] = 0;
					}
				}
			}
		}
		void Daisy::smoothHistogram(std::vector<float>& hist, int hsz){
			int i;
			float prev, temp;
			prev = hist[hsz - 1];
			for (i = 0; i < hsz; i++)
			{
				temp = hist[i];
				hist[i] = (prev + hist[i] + hist[(i + 1 == hsz) ? 0 : i + 1]) / 3.0f;
				prev = temp;
			}
		}
		float Daisy::interpolatePeak(float left, float center, float right){
			if (center < 0.0f)
			{
				left = -left;
				center = -center;
				right = -right;
			}
			float den = (left - 2.0f * center + right);
			if (den == 0) {
				return 0;
			} else {
				return 0.5f*(left - right) / den;
			}
		}
		void Daisy::computeOrientations() {
			Image1f tmp;
			std::vector<Image1f> layers;
			layeredGradient(image,layers,orientationResolutions);
			orientMap.resize(image.width, image.height);
			orientMap.setZero();
			int max_ind;
			int ind;
			float max_val;
			int next, prev;
			float peak, angle;
			float sigma_inc;
			float sigma_prev = 0;
			float sigma_new;
			std::vector<float> hist(orientationResolutions);
			for (int scale = 0; scale<scale_en; scale++){
				sigma_new = std::pow(sigma_step, scale) * descriptorRadius / 3.0f;
				sigma_inc = std::sqrt(sigma_new*sigma_new - sigma_prev*sigma_prev);
				sigma_prev = sigma_new;
				for (Image1f& layer : layers) {
					Smooth<5, 5>(layer, tmp);
					layer = tmp;
				}
				for (int j = 0; j<image.height; j++){
					for (int i = 0; i<image.width; i++){
						if (scaleInvariant &&scaleMap(i,j).x != scale) continue;
						for (int ori = 0; ori<orientationResolutions; ori++){
							hist[ori] = layers[ori](i,j).x;
						}
						for (int kk = 0; kk < 6; kk++) {
							smoothHistogram(hist, orientationResolutions);
						}
						max_val = -1;
						max_ind = 0;
						for (int ori = 0; ori<orientationResolutions; ori++){
							if (hist[ori] > max_val){
								max_val = hist[ori];
								max_ind = ori;
							}
						}
						prev = max_ind - 1;
						if (prev < 0)
							prev += orientationResolutions;
						next = max_ind + 1;
						if (next >=orientationResolutions)
							next -= orientationResolutions;
						peak = interpolatePeak(hist[prev], hist[max_ind], hist[next]);
						angle = (max_ind + peak)*360.0f / orientationResolutions;
						int iangle = (int)std::floor(angle);
						if (iangle <    0) iangle += 360;
						if (iangle >= 360) iangle -= 360;
						if (!(iangle >= 0.0f && iangle < 360.0f)){
							angle = 0;
						}
						orientMap[ind] = iangle;
					}
				}
			}
			computeOrientedGridPoints();
		}
		void Daisy::computeDescriptor(int i,int j, int orientation, Descriptor& out) {

		}
		void Daisy::updateSelectedCubes() {
			for (int r = 0; r<radiusBins; r++){
				float seed_sigma = (r + 1)*descriptorRadius / (radiusBins*0.5f);
				selectedCubes[r] = quantizeRadius(seed_sigma);
			}
		}
		void Daisy::computeCubeSigmas() {
			sigmas.resize(radiusBins);
			float r_step = descriptorRadius / radiusBins;
			for (int r = 0; r< radiusBins; r++){
				sigmas[r] = (r + 1)*r_step / 2.0f;
			}
			updateSelectedCubes();
		}
		void Daisy::computeDescriptors(){
			if (scaleInvariant) computeScales();
			if (rotationInvariant) computeOrientations();
			int orientation;
			descriptorField.resize(image.width, image.height);
			for (int j = 0; j<image.height; j++){
				for (int i = 0; i<image.width; i++)
				{
					orientation = 0;
					if (orientMap.size()>0) orientation = orientMap(i,j).x;
					if (!(orientation >= 0 && orientation < ORIENTATIONS)) orientation = 0;
					computeDescriptor(i,j,orientation, descriptorField(i,j));
				}
			}
		}
		void Daisy::computeOrientedGridPoints() {
			for (int i = 0; i<ORIENTATIONS; i++){
				orientedGridPoints[i].resize(numberOfGridPoints);
				float angle = -i*2.0f*ALY_PI / ORIENTATIONS;
				float cosa = std::cos(angle);
				float sina = std::sin(angle);
				std::vector<float2>& point_list = orientedGridPoints[i];
				for (int k = 0; k<numberOfGridPoints; k++){
					float2 pt = gridPoints[k];
					point_list[k] = float2(pt.x*cosa + pt.y*sina ,-pt.x*sina + pt.y*cosa);
				}
			}
		}
		void Daisy::computeGridPoints() {
			double r_step = descriptorRadius / radiusBins;
			double t_step = 2 * ALY_PI / angleBins;
			gridPoints.resize(numberOfGridPoints);
			for (int i = 0; i<gridPoints.size(); i++){
				gridPoints[i]=float2(0,0);
			}
			for (int r = 0; r<radiusBins; r++){
				int region = r*angleBins + 1;
				for (int t = 0; t<angleBins; t++){
					float x, y;
					polar2cartesian((r + 1)*r_step, t*t_step, x, y);
					gridPoints[region + t]=float2(x,y);
				}
			}
			computeOrientedGridPoints();
		}
	}
}