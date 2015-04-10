#include <cv.hpp>
#include <highgui.h>

#include <StdDeviation.h>

using namespace cv; 
typedef vector<Point> Contour;

inline uchar reduceVal(const uchar val)
{
    if (val < 64)  return 0;
    if (val < 128) return 64;
    if (val < 192) return 128;
    return 192;
}

void quantization(Mat& img)
{
        uchar* pixelPtr = img.data;
        for (int i = 0; i < img.rows; i++)
        for (int j = 0; j < img.cols; j++)
        {
                const int pi = i * img.cols * 3 + j * 3;

                pixelPtr[pi + 0] = reduceVal(pixelPtr[pi + 0]); // B
                pixelPtr[pi + 1] = reduceVal(pixelPtr[pi + 1]); // G
                pixelPtr[pi + 2] = reduceVal(pixelPtr[pi + 2]); // R
        }
}

vector<vector<Point> > contour_detection(Mat& input, int thresh)
{
        vector<vector<Point> > contours;
        {
                Mat threshold_output;
                vector<Vec4i> hierarchy;

                /// Detect edges using Threshold
                
                threshold (input, threshold_output, thresh, 255, THRESH_BINARY );
                findContours (threshold_output, contours, hierarchy, CV_RETR_TREE,
                              CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
        }


        /// Approximate contours to polygons + get bounding rects and circles
        vector<vector<Point> > contours_poly (contours.size());
        for( int i = 0; i < contours.size(); i++ ) {
                approxPolyDP(Mat(contours[i]), contours_poly[i], 1, true);
        }

        return contours_poly;

        // RNG rng(12345);
        // /// Draw polygonal contour + bonding rects + circles
        // for( int i = 0; i < contours.size(); i++ ) {
        //         int tl_y = boundRect[i].tl().y;
        //         int br_y = boundRect[i].br().y;

        //         int c = (tl_y + br_y) * 128 / input.rows;

        //         Scalar color = Scalar (c,c,c);

        //         drawContours (input, contours_poly, i, color, 1, 8,
        //                       vector<Vec4i>(), 0, Point());

        //         rectangle (input, boundRect[i].tl(), boundRect[i].br(),
        //                    color, 2, 8, 0);
        // }




        // cv::imshow("1", input);   
        // cv::waitKey(0);
}

void filter_ratio_constraints(std::vector<Contour>& contours,
                              double min = 0.0, double max = 99999999.99)
{
        contours.erase(
        std::remove_if
        (
                contours.begin(),
                contours.end(),

                [min,max] (Contour& c) -> bool {
                        double ratio;

                        Rect box = boundingRect(c);
                       
                        int x1 = box.tl().x; int x2 = box.br().x;
                        int y1 = box.tl().y; int y2 = box.br().y;

                        double w = std::abs(x1 - x2);
                        double h = std::abs(y1 - y2);

                        return (min > h/w || max < h/w);
                }
        ),
        contours.end());
}

void filter_internal(std::vector<Contour>& contours)
{
        std::size_t size = contours.size();

        std::vector<bool> rm(size);
        for (int i = 0; i < size; i++) {
                rm[i] = false;
        }

        for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
        {
                if (rm[i] || rm[j] || i == j) {
                        continue;
                }

                Rect box_i = boundingRect(contours[i]);
                Rect box_j = boundingRect(contours[j]);

                Point TLi = box_i.tl();
                Point BRi = box_i.br();

                Point TLj = box_j.tl();
                Point BRj = box_j.br();

                int Hi = BRi.y - TLi.y;
                int Wi = BRi.x - TLi.x;

                int Hj = BRj.y - TLj.y;
                int Wj = BRj.x - TLj.x;

                if (TLi.x >= TLj.x && TLi.y >= TLj.y
                &&  BRi.x <= BRj.x && BRi.y <= BRj.y
                &&  Hi*Wi != Hj*Wj) {
                        rm[i] = true;
                } else {
                        rm[i] = false;
                }
        }

        for(int i = size; i >= 0; i--)
        {
                if(rm[i] == true) {
                        contours[i] = contours.back();
                        contours.pop_back();
                }
                std::cout << "rm(" << i << ") = " << rm[i] << "\n";
        }
}

void filter_high_contrast (cv::Mat& input, std::vector<Contour>& contours)
{
        cv::Mat roi;
        for (Contour& c : contours)
        {
                Rect box = boundingRect(c);
                roi      = input(box);

                Point tl = box.tl();
                Point br = box.br();

                std::cout
                        << "TL.x: " << tl.x << " "
                        << "TL.y: " << tl.y << " "
                        << "BR.x: " << br.x << " "
                        << "BR.y: " << br.y << "\n";


                resize(roi, roi, Size(100,100), 0, 0, CV_INTER_AREA );
                cv::imshow("Original", roi);   
             //   cv::imshow("Processed", quant);   
                cv::waitKey(0);

        }
}

int main(int argc, const char * argv[])
{
        // Load input image (colored, 3-channel, BGR)
        cv::Mat input = cv::imread(argv[1]);

        int thresh = std::atoi(argv[2]);

        Mat quant;
        blur  (input, input, Size(3,3));
      //  quantization(input);
        cvtColor (input, quant, CV_BGR2GRAY);
       
        int threshold = 100;

//        Canny (quant, quant, threshold, 3*threshold, 3);
        Canny (quant, quant, threshold, threshold*3, 3);


        // int block_size = 2;
        // int aperture_size = 31;
        // double k = 3;

        // Mat dst, dst_norm, dst_norm_scaled;
        // cornerHarris(quant, dst, block_size, aperture_size, k, BORDER_DEFAULT);
        
        // // Normalizing
        // normalize( dst, dst_norm, 0, 255, NORM_MINMAX, CV_32FC1, Mat() );
        // convertScaleAbs( dst_norm, dst_norm_scaled );

        // dst_norm_scaled.copyTo(quant);

        // quantization(quant);
        vector<Contour> contours = contour_detection(quant, thresh);

        // finds letters and numbers via font ratio
        filter_ratio_constraints(contours, 1.5, 5);

        filter_internal(contours);

        // removes low contrast boxes not likely to contain plate info
        //filter_high_contrast(input, contours);

        for (int i = 0; i < contours.size(); i++)
        {
                Rect box   = boundingRect(contours[i]);

                int height = (box.br().y - box.tl().y);
                int width  = (box.br().x - box.tl().x);
                int cy     = (box.tl().y + height / 2);

                {
                        // Scalar color = Scalar (
                        //         (height * width * cy) / (1.5 * quant.rows * quant.cols) * 255
                        //         );
                        Scalar color = Scalar (
                                height * 255 / quant.rows,
                                height * 255 / quant.rows,
                                height * 255 / quant.rows);

                        // if (argc > 3) {
                        //         std::cout << "Saving ROI\n";
                        //         imwrite("roi/" + std::to_string(i) + ".jpg", input(box)); // A JPG FILE IS BEING SAVED
                        // } else {
                        //rectangle (quant, box.tl(), box.br(), Scalar(255,0,0), 2, 8, 0);
                        rectangle (input, box.tl(), box.br(), color, 2, 8, 0);
                        //}
                        //drawContours (input,
                        //        contours /* array of arrays */, i /* index */,
                        //        color, 4 /* thickness */,   8 /* linetype */,
                        //        vector<Vec4i>(), 8 /* maxlevel */,
                        //        Point()
                        //);
                }
        }
        std::cout << "--- DONE ---\n";

        cv::imshow("Original", input);   
     //   cv::imshow("Processed", quant);   
        cv::waitKey(0);

        return 0;
}