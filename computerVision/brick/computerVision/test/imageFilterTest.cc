/**
***************************************************************************
* @file filterTest.cpp
*
* Source file defining tests for filter().
*
* Copyright (C) 2006 David LaRose, dlr@cs.cmu.edu
* See accompanying file, LICENSE.TXT, for details.
*
***************************************************************************
**/

#include <brick/common/functional.hh>
#include <brick/computerVision/test/testImages.hh>
#include <brick/computerVision/image.hh>
#include <brick/computerVision/imageFilter.hh>
#include <brick/computerVision/imageIO.hh>
#include <brick/computerVision/kernel.hh>
#include <brick/computerVision/utilities.hh>
#include <brick/numeric/utilities.hh>
#include <brick/test/testFixture.hh>

// xxx
#include<brick/utilities/timeUtilities.hh>

namespace brick {

  namespace computerVision {
    
    class FilterTest : public brick::test::TestFixture<FilterTest> {

    public:

      FilterTest();
      ~FilterTest() {}

      void setUp(const std::string& /* testName */) {}
      void tearDown(const std::string& /* testName */) {}

      // Tests of member functions.
      void testFilter2D_nonSeparable_i();
      void testFilter2D_nonSeparable();
      void testFilter2D_separable_i();
      void testFilter2D_separable();

      // XXX Temporary tests.
      void testBinomialRowFiltering();

    private:

      numeric::Array2D<common::UnsignedInt8>
      localFilter2D(const numeric::Array2D<common::Float64>& kernel,
                    const numeric::Array2D<common::UnsignedInt8>& inputImage);

    
      numeric::Array2D<common::Float64>
      localFilter2D(const numeric::Array2D<common::Float64>& kernel,
                    const numeric::Array2D<common::Float64>& inputImage);

      numeric::Array2D<common::UnsignedInt64>
      localFilter2D(const numeric::Array2D<common::UnsignedInt64>& kernel,
                    const numeric::Array2D<common::UnsignedInt8>& inputImage);

    }; // class FilterTest


    /* ============== Member Function Definititions ============== */

    FilterTest::
    FilterTest()
      : brick::test::TestFixture<FilterTest>("FilterTest")
    {
      BRICK_TEST_REGISTER_MEMBER(testFilter2D_nonSeparable_i);
      BRICK_TEST_REGISTER_MEMBER(testFilter2D_nonSeparable);
      BRICK_TEST_REGISTER_MEMBER(testFilter2D_separable_i);
      BRICK_TEST_REGISTER_MEMBER(testFilter2D_separable);

      // xxx
      BRICK_TEST_REGISTER_MEMBER(testBinomialRowFiltering);
    }


    void
    FilterTest::
    testFilter2D_nonSeparable_i()
    {
      Image<GRAY_FLOAT64> inputImage0(
	numeric::Array2D<common::Float64>("[[1.0, 2.0, 3.0, 4.0, 5.0],"
			 " [6.0, 7.0, 8.0, 9.0, 10.0],"
			 " [9.0, 8.0, 7.0, 6.0, 5.0],"
			 " [4.0, 3.0, 2.0, 1.0, 0.0]]"));
      numeric::Array2D<double> kernelData("[[1.0, 2.0, 1.0],"
                                 " [2.0, 4.0, 2.0],"
                                 " [3.0, 5.0, 4.0]]");
      // kernelData /= numeric::sum<double>(ravel(kernelData));
      Kernel<double> kernel0(kernelData);
      Image<GRAY_FLOAT64> resultImage =
	brick::computerVision::filter2D<
	GRAY_FLOAT64, GRAY_FLOAT64, common::Float64>(
	  kernel0, inputImage0, static_cast<common::Float64>(0));
      Image<GRAY_FLOAT64> referenceImage = localFilter2D(
        kernel0.getArray2D(), inputImage0);

      BRICK_TEST_ASSERT(resultImage.rows() == referenceImage.rows());
      BRICK_TEST_ASSERT(resultImage.columns() == referenceImage.columns());
      double tolerance = 1.0E-12;
      for(size_t index0 = 0; index0 < resultImage.size(); ++index0) {
        BRICK_TEST_ASSERT(
          approximatelyEqual(
            resultImage[index0], referenceImage[index0], tolerance));
      }
    }


    void
    FilterTest::
    testFilter2D_nonSeparable()
    {
      Image<GRAY8> inputImage0 = readPGM8(getTestImageFileNamePGM0());
      numeric::Array2D<double> kernelData("[[1.0, 2.0, 1.0],"
                                 " [2.0, 4.0, 2.0],"
                                 " [3.0, 5.0, 4.0],"
                                 " [2.0, 5.0, 1.0],"
                                 " [0.0, 2.0, 1.0]]");
      kernelData /= numeric::sum<double>(ravel(kernelData));
      Kernel<double> kernel0(kernelData);
      // Note(xxx): Need to make this not require explicit template
      // parameters.
      Image<GRAY_FLOAT64> tempImage =
        brick::computerVision::filter2D<GRAY_FLOAT64, GRAY8, double>(
          kernel0, inputImage0, 0.0);
      Image<GRAY8> resultImage =
        brick::computerVision::convertColorspace<GRAY8>(tempImage);
      Image<GRAY8> referenceImage = localFilter2D(
        kernel0.getArray2D(), inputImage0);

      BRICK_TEST_ASSERT(resultImage.rows() == referenceImage.rows());
      BRICK_TEST_ASSERT(resultImage.columns() == referenceImage.columns());
      BRICK_TEST_ASSERT(std::equal(resultImage.begin(), resultImage.end(),
                                 referenceImage.begin()));
    }


    void
    FilterTest::
    testFilter2D_separable_i()
    {
      Image<GRAY_FLOAT64> inputImage0(
	numeric::Array2D<common::Float64>("[[1.0, 2.0, 3.0, 4.0, 5.0],"
			 " [6.0, 7.0, 8.0, 9.0, 10.0],"
			 " [9.0, 8.0, 7.0, 6.0, 5.0],"
			 " [4.0, 3.0, 2.0, 1.0, 0.0]]"));

      numeric::Array1D<double> kernelRow("[1.0, 3.0, 2.0]");
      numeric::Array1D<double> kernelColumn("[2.0, 0.0, 1.0]");
      Kernel<double> kernel0(kernelRow, kernelColumn);

      numeric::Array2D<double> referenceKernelArray(
        kernelColumn.size(), kernelRow.size());
      for(size_t rowIndex = 0; rowIndex < referenceKernelArray.rows();
          ++rowIndex) {
        for(size_t columnIndex = 0;
	    columnIndex < referenceKernelArray.columns(); ++columnIndex) {
          referenceKernelArray(rowIndex, columnIndex) =
            kernelColumn[rowIndex] * kernelRow[columnIndex];
        }
      }


      Image<GRAY_FLOAT64> resultImage =
	brick::computerVision::filter2D<
	GRAY_FLOAT64, GRAY_FLOAT64, common::Float64>(
	  kernel0, inputImage0, static_cast<common::Float64>(0));
      Image<GRAY_FLOAT64> referenceImage = localFilter2D(
        kernel0.getArray2D(), inputImage0);

      BRICK_TEST_ASSERT(resultImage.rows() == referenceImage.rows());
      BRICK_TEST_ASSERT(resultImage.columns() == referenceImage.columns());
      double tolerance = 1.0E-12;
      for(size_t index0 = 0; index0 < resultImage.size(); ++index0) {
        BRICK_TEST_ASSERT(
          approximatelyEqual(
            resultImage[index0], referenceImage[index0], tolerance));
      }
    }


    void
    FilterTest::
    testFilter2D_separable()
    {
      Image<GRAY8> inputImage = readPGM8(getTestImageFileNamePGM0());
      Image<GRAY_FLOAT64> inputImage0(inputImage.rows(), inputImage.columns());
      inputImage0.copy(inputImage);
    
      numeric::Array1D<double> kernelRow("[1.0, 3.0, 4.0]");
      numeric::Array1D<double> kernelColumn("[3.0, 0.0, 2.0, 4.0, 1.0]");
      kernelRow /= numeric::sum<double>(kernelRow);
      kernelColumn /= numeric::sum<double>(kernelColumn);
      Kernel<double> kernel0(kernelRow, kernelColumn);

      numeric::Array2D<double> referenceKernelArray(
        kernelColumn.size(), kernelRow.size());
      for(size_t rowIndex = 0; rowIndex < referenceKernelArray.rows();
          ++rowIndex) {
        for(size_t columnIndex = 0; columnIndex < referenceKernelArray.columns();
            ++columnIndex) {
          referenceKernelArray(rowIndex, columnIndex) =
            kernelColumn[rowIndex] * kernelRow[columnIndex];
        }
      }

      // Note(xxx): Need to make this not require explicit template
      // parameters.
      Image<GRAY_FLOAT64> resultImage =
        computerVision::filter2D<
	GRAY_FLOAT64, GRAY_FLOAT64, double>(
          kernel0, inputImage0, static_cast<common::Float64>(0));
      Image<GRAY_FLOAT64> referenceImage = localFilter2D(
        referenceKernelArray, inputImage0);

      double tolerance = 1.0E-12;
      for(size_t index0 = 0; index0 < resultImage.size(); ++index0) {
        BRICK_TEST_ASSERT(
          approximatelyEqual(
            resultImage[index0], referenceImage[index0], tolerance));
      }
    }


    void
    FilterTest::
    testBinomialRowFiltering()
    {
      Image<GRAY8> inputImage = readPGM8(getTestImageFileNamePGM0());
      Image<GRAY16> image16bit = convertColorspace<GRAY16>(inputImage);

      Image<GRAY16> outputImageMultiPass;
      double time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        Image<GRAY16> tempImage = privateCode::filterRows121(
          image16bit, 1, 0, 0);
        outputImageMultiPass = privateCode::filterRows121(
          tempImage, 1, 0, 4);
      }
      double time1 = utilities::getCurrentTime();
      std::cout << "Multi-pass ET: " << time1 - time0 << std::endl;

      Image<GRAY16> outputImage121;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputImage121 = privateCode::filterRows121(image16bit, 2, 0);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "121 x 2 ET: " << time1 - time0 << std::endl;
      
      Image<GRAY16> outputImage14641;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputImage14641 = privateCode::filterRows14641(image16bit, 0);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "14641 ET: " << time1 - time0 << std::endl;

      for(unsigned int rr = 0; rr < inputImage.rows(); ++rr) {
        for(unsigned int cc = 2; cc < inputImage.rows() - 2; ++cc) {
          BRICK_TEST_ASSERT(
            outputImageMultiPass(rr, cc) == outputImage121(rr, cc));
          BRICK_TEST_ASSERT(
            outputImageMultiPass(rr, cc) == outputImage14641(rr, cc));
        }
      }


      Image<GRAY16> outputGenericRow;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputGenericRow = privateCode::filter_1_6_15_20_15_6_1(
          image16bit, 1, 0, image16bit.rows(), 3, image16bit.columns() - 3);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "Generic 1 6 15 20 15 6 1: " << time1 - time0 << std::endl;

      Image<GRAY16> outputGenericColumn;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputGenericColumn = privateCode::filter_1_6_15_20_15_6_1(
          image16bit, image16bit.columns(), 3, image16bit.rows() - 3,
          0, image16bit.columns());
      }
      time1 = utilities::getCurrentTime();
      std::cout << "Generic Col 1 6 15 20 15 6 1: " << time1 - time0
                << std::endl;

      
      Image<GRAY16> outputImage1_6_15_20_15_6_1;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputImage1_6_15_20_15_6_1 =
          privateCode::filterRows1_6_15_20_15_6_1(image16bit, 0);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "1_6_15_20_15_6_1 ET: " << time1 - time0 << std::endl;

      
      Image<GRAY16> outputRowSimple;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputRowSimple = privateCode::filterRows121_simple(image16bit, 0, -1);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "RowSimple ET: " << time1 - time0 << std::endl;

      Image<GRAY16> outputCol;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputCol = privateCode::filterColumns121(image16bit, 0, -1);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "Col ET: " << time1 - time0 << std::endl;

      Image<GRAY16> outputCol14641;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputCol14641 = privateCode::filterColumns14641(image16bit, 0, -1);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "Col 14641 ET: " << time1 - time0 << std::endl;

      Image<GRAY16> outputCol1_6_15_20_15_6_1;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputCol1_6_15_20_15_6_1 =
          privateCode::filterColumns1_6_15_20_15_6_1(image16bit, 0, -1);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "Col 1_6_15_20_15_6_1 ET: " << time1 - time0 << std::endl;

      Image<GRAY16> outputColSimple;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
        outputColSimple = privateCode::filterColumns121_simple(
          image16bit, 0, -1);
      }
      time1 = utilities::getCurrentTime();
      std::cout << "ColSimple ET: " << time1 - time0 << std::endl;


      Image<GRAY_FLOAT32> inputFloat32 = convertColorspace<GRAY_FLOAT32>(
        image16bit);
      // numeric::Array1D<common::Float32> kernelRow32("[1.0, 2.0, 1.0]");
      // numeric::Array1D<common::Float32> kernelColumn32("[1.0, 2.0, 1.0]");
      numeric::Array1D<common::Float32> kernelRow32(
        "[1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0]");
      numeric::Array1D<common::Float32> kernelColumn32(
        "[1.0, 6.0, 15.0, 20.0, 15.0, 6.0, 1.0]");
      Kernel<common::Float32> kernelFloat32(kernelRow32, kernelColumn32);
      Image<GRAY_FLOAT32> outputFloat32;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
	outputFloat32 = brick::computerVision::filter2D<
          GRAY_FLOAT32, GRAY_FLOAT32, common::Float32>(
            kernelFloat32, inputFloat32, static_cast<common::Float32>(0));
      }
      time1 = utilities::getCurrentTime();
      std::cout << "FloatSeparable: " << time1 - time0 << std::endl;

      Image<GRAY_FLOAT64> inputFloat64 = convertColorspace<GRAY_FLOAT64>(
        image16bit);
      numeric::Array1D<common::Float64> kernelRow64("[1.0, 2.0, 1.0]");
      numeric::Array1D<common::Float64> kernelColumn64("[1.0, 2.0, 1.0]");
      Kernel<common::Float64> kernelFloat64(kernelRow64, kernelColumn64);
      Image<GRAY_FLOAT64> outputFloat64;
      time0 = utilities::getCurrentTime();
      for(unsigned int ii = 0; ii < 100; ++ii) {
	outputFloat64 = brick::computerVision::filter2D<
          GRAY_FLOAT64, GRAY_FLOAT64, common::Float64>(
            kernelFloat64, inputFloat64, static_cast<common::Float64>(0));
      }
      time1 = utilities::getCurrentTime();
      std::cout << "DoubleSeparable: " << time1 - time0 << std::endl;
      
    }



    numeric::Array2D<common::UnsignedInt8>
    FilterTest::
    localFilter2D(const numeric::Array2D<common::Float64>& kernel,
                  const numeric::Array2D<common::UnsignedInt8>& inputImage)
    {
      size_t halfKernelRows = kernel.rows() / 2;
      size_t halfKernelColumns = kernel.columns() / 2;
      numeric::Array2D<common::UnsignedInt8> resultImage(inputImage.rows(), inputImage.columns());
      for(size_t imageRow = 0; imageRow < inputImage.rows(); ++imageRow) {
        for(size_t imageColumn = 0; imageColumn < inputImage.columns();
            ++imageColumn) {
          if(imageRow < halfKernelRows
             || imageColumn < halfKernelColumns
             || imageRow >= inputImage.rows() - halfKernelRows
             || imageColumn >= inputImage.columns() - halfKernelColumns) {
            resultImage(imageRow, imageColumn) = static_cast<common::UnsignedInt8>(0);
            continue;
          }
          common::Float64 dotProduct = 0.0;
          for(size_t kernelRow = 0; kernelRow < kernel.rows(); ++kernelRow) {
            for(size_t kernelColumn = 0; kernelColumn < kernel.columns();
                ++kernelColumn) {
              dotProduct += (
                inputImage(imageRow - halfKernelRows + kernelRow,
                           imageColumn - halfKernelColumns + kernelColumn)
                * kernel(kernelRow, kernelColumn));
            }
          }
          resultImage(imageRow, imageColumn) =
//             static_cast<common::UnsignedInt8>(dotProduct + 0.5);
	    static_cast<common::UnsignedInt8>(dotProduct);
        }
      }
      return resultImage;
    }


    numeric::Array2D<common::Float64>
    FilterTest::
    localFilter2D(const numeric::Array2D<common::Float64>& kernel,
                  const numeric::Array2D<common::Float64>& inputImage)
    {
      size_t halfKernelRows = kernel.rows() / 2;
      size_t halfKernelColumns = kernel.columns() / 2;
      numeric::Array2D<common::Float64> resultImage(inputImage.rows(), inputImage.columns());
      for(size_t imageRow = 0; imageRow < inputImage.rows(); ++imageRow) {
        for(size_t imageColumn = 0; imageColumn < inputImage.columns();
            ++imageColumn) {
          if(imageRow < halfKernelRows
             || imageColumn < halfKernelColumns
             || imageRow >= inputImage.rows() - halfKernelRows
             || imageColumn >= inputImage.columns() - halfKernelColumns) {
            resultImage(imageRow, imageColumn) = static_cast<common::Float64>(0);
            continue;
          }
          common::Float64 dotProduct = 0.0;
          for(size_t kernelRow = 0; kernelRow < kernel.rows(); ++kernelRow) {
            for(size_t kernelColumn = 0; kernelColumn < kernel.columns();
                ++kernelColumn) {
              dotProduct += (
                inputImage(imageRow - halfKernelRows + kernelRow,
                           imageColumn - halfKernelColumns + kernelColumn)
                * kernel(kernelRow, kernelColumn));
            }
          }
          resultImage(imageRow, imageColumn) =
            static_cast<common::Float64>(dotProduct);
        }
      }
      return resultImage;
    }  


    numeric::Array2D<common::UnsignedInt64>
    FilterTest::
    localFilter2D(const numeric::Array2D<common::UnsignedInt64>& kernel,
                  const numeric::Array2D<common::UnsignedInt8>& inputImage)
    {
      size_t halfKernelRows = kernel.rows() / 2;
      size_t halfKernelColumns = kernel.columns() / 2;
      numeric::Array2D<common::UnsignedInt64> resultImage(inputImage.rows(), inputImage.columns());
      for(size_t imageRow = 0; imageRow < inputImage.rows(); ++imageRow) {
        for(size_t imageColumn = 0; imageColumn < inputImage.columns();
            ++imageColumn) {
          if(imageRow < halfKernelRows
             || imageColumn < halfKernelColumns
             || imageRow >= inputImage.rows() - halfKernelRows
             || imageColumn >= inputImage.columns() - halfKernelColumns) {
            resultImage(imageRow, imageColumn) = static_cast<common::UnsignedInt64>(0);
            continue;
          }
          common::UnsignedInt64 dotProduct = 0;
          for(size_t kernelRow = 0; kernelRow < kernel.rows(); ++kernelRow) {
            for(size_t kernelColumn = 0; kernelColumn < kernel.columns();
                ++kernelColumn) {
              dotProduct += (
                inputImage(imageRow - halfKernelRows + kernelRow,
                           imageColumn - halfKernelColumns + kernelColumn)
                * kernel(kernelRow, kernelColumn));
            }
          }
          resultImage(imageRow, imageColumn) = dotProduct;
        }
      }
      return resultImage;
    }  

  } // namespace computerVision

} // namespace brick


#if 0

int main(int argc, char** argv)
{
  brick::computerVision::FilterTest currentTest;
  bool result = currentTest.run();
  return (result ? 0 : 1);
}

#else

namespace {

  brick::computerVision::FilterTest currentTest;

}

#endif
