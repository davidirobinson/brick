/**
***************************************************************************
* @file calibrationToolsTest.cpp
*
* Source file defining tests for the calibration utility routines.
*
* Copyright (C) 2009-2012 David LaRose, dlr@cs.cmu.edu
* See accompanying file, LICENSE.TXT, for details.
*
***************************************************************************
**/


#include <brick/common/functional.hh>
#include <brick/computerVision/calibrationTools.hh>
#include <brick/computerVision/cameraIntrinsicsPinhole.hh>
#include <brick/computerVision/cameraIntrinsicsPlumbBob.hh>
#include <brick/computerVision/image.hh>
#include <brick/geometry/utilities3D.hh>
#include <brick/numeric/transform3D.hh>
#include <brick/numeric/quaternion.hh>
#include <brick/numeric/rotations.hh>
#include <brick/test/testFixture.hh>

using namespace brick::common;
using namespace brick::computerVision;
using namespace brick::geometry;
using namespace brick::numeric;
using namespace brick::test;


class CalibrationToolsTest
  : public TestFixture<CalibrationToolsTest> {

public:

  CalibrationToolsTest();
  ~CalibrationToolsTest() {}

  void setUp(const std::string& /* testName */) {}
  void tearDown(const std::string& /* testName */) {}

  // Tests.
  void testCameraParametersObjectiveFunction();
  void testEstimateCameraIntrinsics();
  void testEstimateCameraIntrinsicsPinhole();
  void testEstimateCameraParameters();
  void testEstimateCameraParametersConstrained();
  void testEstimateCameraParametersPinhole();
  void testEstimateTransform3DTo2D();
  void testEstimateProjectedAreaAndCentroid();

private:

  bool
  checkIntrinsicsEqual(CameraIntrinsicsPinhole<double> const& intrinsics0,
                       CameraIntrinsicsPinhole<double> const& intrinsics1,
                       double tolerance);

  bool
  checkIntrinsicsEqual(CameraIntrinsicsPlumbBob<double> const& intrinsics0,
                       CameraIntrinsicsPlumbBob<double> const& intrinsics1,
                       double tolerance);


  bool
  checkTransformEqual(Transform3D<double> const& transform0,
                      Transform3D<double> const& transform1,
                      double tolerance);

  bool
  checkVectorEqual(Array1D<double> const& vector0,
                   Array1D<double> const& vector1,
                   double tolerance);


  template <class Intrinsics>
  void
  compute2DTestData(std::vector< Vector2D<double> >& points2D,
                    std::vector< Vector3D<double> >& points3D_camera,
                    Intrinsics const& intrinsics);

  void
  get3DTestData(std::vector< Vector3D<double> >& points3D_world,
                std::vector< Vector3D<double> >& points3D_camera,
                Transform3D<double>& cameraTworld);

  void
  getTestIntrinsicsPlumbBob(CameraIntrinsicsPlumbBob<double>& intrinsics,
                            double resolutionFactor = 1.0);


  double m_defaultTolerance;
  double m_relaxedTolerance;
  double m_stringentTolerance;

}; // class CalibrationToolsTest


/* ============== Member Function Definititions ============== */

CalibrationToolsTest::
CalibrationToolsTest()
  : brick::test::TestFixture<CalibrationToolsTest>("CalibrationToolsTest"),
    m_defaultTolerance(1.0E-10),
    m_relaxedTolerance(5.0E-5),
    m_stringentTolerance(1.0E-13)
{
  BRICK_TEST_REGISTER_MEMBER(testCameraParametersObjectiveFunction);
  BRICK_TEST_REGISTER_MEMBER(testEstimateCameraIntrinsics);
  BRICK_TEST_REGISTER_MEMBER(testEstimateCameraIntrinsicsPinhole);
  BRICK_TEST_REGISTER_MEMBER(testEstimateCameraParameters);
  BRICK_TEST_REGISTER_MEMBER(testEstimateCameraParametersConstrained);
  BRICK_TEST_REGISTER_MEMBER(testEstimateCameraParametersPinhole);
  BRICK_TEST_REGISTER_MEMBER(testEstimateTransform3DTo2D);
  BRICK_TEST_REGISTER_MEMBER(testEstimateProjectedAreaAndCentroid);
}


void
CalibrationToolsTest::
testCameraParametersObjectiveFunction()
{
  typedef brick::computerVision::privateCode::CameraParametersObjectiveFunction<
    CameraIntrinsicsPlumbBob<double> > ObjectiveFunction;

  // Get some test data.
  CameraIntrinsicsPlumbBob<double> referenceIntrinsics;
  this->getTestIntrinsicsPlumbBob(referenceIntrinsics);
  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, referenceIntrinsics);
  Quaternion<double> cameraQworld = transform3DToQuaternion(cameraTworld);

  // Verify that, given perfect parameters, we get nearly-zero error.
  CameraIntrinsicsPlumbBob<double> zeroTestIntrinsics = referenceIntrinsics;
  CameraIntrinsicsPlumbBob<double>::ParameterVectorType intrinsicsFreeParameters =
    zeroTestIntrinsics.getFreeParameters();
  CameraIntrinsicsPlumbBob<double>::ParameterVectorType allFreeParameters(
    intrinsicsFreeParameters.size() + 7);
  allFreeParameters[0] = cameraQworld.s();
  allFreeParameters[1] = cameraQworld.i();
  allFreeParameters[2] = cameraQworld.j();
  allFreeParameters[3] = cameraQworld.k();
  allFreeParameters[4] = cameraTworld(0, 3);
  allFreeParameters[5] = cameraTworld(1, 3);
  allFreeParameters[6] = cameraTworld(2, 3);
  std::copy(intrinsicsFreeParameters.begin(), intrinsicsFreeParameters.end(),
            allFreeParameters.begin() + 7);
  ObjectiveFunction objectiveFunction(
    zeroTestIntrinsics, points3D_world.begin(), points3D_world.end(),
    points2D.begin());
  typedef brick::optimization::GradientFunctionLM<ObjectiveFunction>
    GradientFunctionLM;
  GradientFunctionLM gradientFunction(objectiveFunction);
  double nominalZeroValue = gradientFunction(allFreeParameters);

  // In addition to calculating error values, we need
  // objectiveFunction to expose an interface for finding the
  // intrinsics and extrinsics that correspond to the minimum
  // objective value.  Here we'll call setParameters() with the
  // perfect parameters, and make sure that objectiveFunction converts
  // them to the correct intrinsics and extrinsics.
  objectiveFunction.setParameters(allFreeParameters);
  zeroTestIntrinsics = objectiveFunction.getIntrinsics();
  Transform3D<double> zeroTestCameraTworld = objectiveFunction.getPoseCameraTworld();
  CameraIntrinsicsPlumbBob<double>::ParameterVectorType intrinsicsFreeParameters2 =
    zeroTestIntrinsics.getFreeParameters();
  BRICK_TEST_ASSERT(
    this->checkVectorEqual(intrinsicsFreeParameters, intrinsicsFreeParameters2,
                           m_stringentTolerance));
  BRICK_TEST_ASSERT(
    this->checkIntrinsicsEqual(
      zeroTestIntrinsics, referenceIntrinsics, m_relaxedTolerance));
  BRICK_TEST_ASSERT(
    this->checkTransformEqual(
      zeroTestCameraTworld, cameraTworld, m_stringentTolerance));

  // Make sure the resulting objective value is close to zero.
  BRICK_TEST_ASSERT(
    approximatelyEqual(nominalZeroValue, 0.0, m_stringentTolerance));

  // Make sure objective > 0 when at non-ideal parameters.
  for(unsigned int ii = 0; ii < allFreeParameters.size(); ++ii) {
    CameraIntrinsicsPlumbBob<double>::ParameterVectorType tempParameters(
      allFreeParameters.size());
    std::copy(allFreeParameters.begin(), allFreeParameters.end(),
              tempParameters.begin());

    tempParameters[ii] += 0.01;
    double nonzeroValue = gradientFunction(tempParameters);
    // std::cout << ii << ": " << nonzeroValue << std::endl;
    BRICK_TEST_ASSERT(nonzeroValue > m_defaultTolerance);

    tempParameters[ii] -= 0.02;
    nonzeroValue = gradientFunction(tempParameters);
    // std::cout << ii << "(-): " << nonzeroValue << std::endl;
    BRICK_TEST_ASSERT(nonzeroValue > m_defaultTolerance);
  }

}


void
CalibrationToolsTest::
testEstimateCameraIntrinsics()
{
  CameraIntrinsicsPlumbBob<double> referenceIntrinsics;
  this->getTestIntrinsicsPlumbBob(referenceIntrinsics);

  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, referenceIntrinsics);

  CameraIntrinsicsPlumbBob<double> recoveredIntrinsics =
    estimateCameraIntrinsics< CameraIntrinsicsPlumbBob<double> >(
      referenceIntrinsics.getNumPixelsX(), referenceIntrinsics.getNumPixelsY(),
      points3D_camera.begin(), points3D_camera.end(), points2D.begin());

  BRICK_TEST_ASSERT(
    this->checkIntrinsicsEqual(
      recoveredIntrinsics, referenceIntrinsics, m_relaxedTolerance));
}


void
CalibrationToolsTest::
testEstimateCameraIntrinsicsPinhole()
{
  // Arbitrary camera params.
  size_t numPixelsX = 320;
  size_t numPixelsY = 240;
  double focalLength = 0.03;
  double pixelSizeX = 0.001;
  double pixelSizeY = 0.002;
  double centerU = 100.0;
  double centerV = 125.0;
  CameraIntrinsicsPinhole<double> intrinsics(
    numPixelsX, numPixelsY, focalLength, pixelSizeX, pixelSizeY,
    centerU, centerV);

  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, intrinsics);

  CameraIntrinsicsPinhole<double> recoveredIntrinsics =
    estimateCameraIntrinsicsPinhole<double>(
      intrinsics.getNumPixelsX(), intrinsics.getNumPixelsY(),
      points3D_camera.begin(), points3D_camera.end(), points2D.begin());

  BRICK_TEST_ASSERT(
    this->checkIntrinsicsEqual(
      recoveredIntrinsics, intrinsics, m_relaxedTolerance));
}


void
CalibrationToolsTest::
testEstimateCameraParameters()
{
  CameraIntrinsicsPlumbBob<double> referenceIntrinsics;
  this->getTestIntrinsicsPlumbBob(referenceIntrinsics);

  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, referenceIntrinsics);

  // Now run the estimation code.
  CameraIntrinsicsPlumbBob<double> recoveredIntrinsics;
  Transform3D<double> recoveredCameraTworld;
  CameraParameterEstimationStatistics<double> statistics;
  estimateCameraParameters< CameraIntrinsicsPlumbBob<double> >(
    recoveredIntrinsics, recoveredCameraTworld, statistics,
    referenceIntrinsics.getNumPixelsX(), referenceIntrinsics.getNumPixelsY(),
    points3D_world.begin(), points3D_world.end(), points2D.begin());

#if 0
  std::cout << "Unconstrained condition: "
            << statistics.getConditionNumber() << std::endl;
  std::cout << "Parameters:\n" << statistics.getParameters()
            << "\n" << std::endl;
  std::cout << "Eigenvalues:\n" << statistics.getEigenvalues()
            << "\n" << std::endl;
  std::cout << "Eigenvectors (per row):\n"
            << statistics.getEigenvectors().transpose()
            << "\n" << std::endl;
#endif

  BRICK_TEST_ASSERT(statistics.getConditionNumber() < 1.0E8);
  BRICK_TEST_ASSERT(
    this->checkIntrinsicsEqual(
      recoveredIntrinsics, referenceIntrinsics, m_relaxedTolerance));
  BRICK_TEST_ASSERT(this->checkTransformEqual(
                    recoveredCameraTworld, cameraTworld, m_relaxedTolerance));
}


void
CalibrationToolsTest::
testEstimateCameraParametersConstrained()
{
  CameraIntrinsicsPlumbBob<double> referenceIntrinsics;
  this->getTestIntrinsicsPlumbBob(referenceIntrinsics);
  referenceIntrinsics.allowSixthOrderRadial(false);
  referenceIntrinsics.allowSkew(false);

  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, referenceIntrinsics);

  // Now run the estimation code.
  CameraIntrinsicsPlumbBob<double> recoveredIntrinsics;
  Transform3D<double> recoveredCameraTworld;
  recoveredIntrinsics.allowSkew(false);
  recoveredIntrinsics.allowSixthOrderRadial(false);
  CameraParameterEstimationStatistics<double> statistics;
  estimateCameraParameters< CameraIntrinsicsPlumbBob<double> >(
    recoveredIntrinsics, recoveredCameraTworld, statistics,
    referenceIntrinsics.getNumPixelsX(), referenceIntrinsics.getNumPixelsY(),
    points3D_world.begin(), points3D_world.end(), points2D.begin());

#if 0
  std::cout << "Constrained condition: "
            << statistics.getConditionNumber() << "\n" << std::endl;
  std::cout << "Parameters:\n" << statistics.getParameters()
            << "\n" << std::endl;
  std::cout << "Eigenvalues:\n" << statistics.getEigenvalues()
            << "\n" << std::endl;
  std::cout << "Eigenvectors (per row):\n"
            << statistics.getEigenvectors().transpose()
            << "\n" << std::endl;
#endif

  BRICK_TEST_ASSERT(statistics.getConditionNumber() < 1.0E8);
  BRICK_TEST_ASSERT(recoveredIntrinsics.getSkewCoefficient() == 0.0);
  BRICK_TEST_ASSERT(recoveredIntrinsics.getRadialCoefficient2() == 0.0);
  BRICK_TEST_ASSERT(
    this->checkIntrinsicsEqual(
      recoveredIntrinsics, referenceIntrinsics, m_relaxedTolerance));
  BRICK_TEST_ASSERT(this->checkTransformEqual(
                    recoveredCameraTworld, cameraTworld, m_relaxedTolerance));
}


void
CalibrationToolsTest::
testEstimateCameraParametersPinhole()
{
  // Arbitrary camera params.
  size_t numPixelsX = 320;
  size_t numPixelsY = 240;
  double focalLength = 0.03;
  double pixelSizeX = 0.001;
  double pixelSizeY = 0.002;
  double centerU = 100.0;
  double centerV = 125.0;
  CameraIntrinsicsPinhole<double> intrinsics(
    numPixelsX, numPixelsY, focalLength, pixelSizeX, pixelSizeY,
    centerU, centerV);

  // Get some data with which to test.
  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, intrinsics);

  // The "easy" case, where cameraTworld is identity.
  CameraIntrinsicsPinhole<double> recoveredIntrinsics0;
  brick::numeric::Transform3D<double> recoveredCameraTworld0;
  estimateCameraParametersPinhole(
    recoveredIntrinsics0, recoveredCameraTworld0,
    intrinsics.getNumPixelsX(), intrinsics.getNumPixelsY(),
    points3D_camera.begin(), points3D_camera.end(), points2D.begin());
  BRICK_TEST_ASSERT(this->checkIntrinsicsEqual(
                    recoveredIntrinsics0, intrinsics, m_relaxedTolerance));
  Transform3D<double> identityXf;
  BRICK_TEST_ASSERT(this->checkTransformEqual(
                    recoveredCameraTworld0, identityXf, m_relaxedTolerance));

  // The "hard" case, where cameraTworld is not identity.
  CameraIntrinsicsPinhole<double> recoveredIntrinsics1;
  brick::numeric::Transform3D<double> recoveredCameraTworld1;
  estimateCameraParametersPinhole(
    recoveredIntrinsics1, recoveredCameraTworld1,
    intrinsics.getNumPixelsX(), intrinsics.getNumPixelsY(),
    points3D_world.begin(), points3D_world.end(), points2D.begin());
  BRICK_TEST_ASSERT(this->checkIntrinsicsEqual(
                    recoveredIntrinsics1, intrinsics, m_relaxedTolerance));
  BRICK_TEST_ASSERT(this->checkTransformEqual(
                    recoveredCameraTworld1, cameraTworld, m_relaxedTolerance));
}


void
CalibrationToolsTest::
testEstimateTransform3DTo2D()
{
  // Arbitrary camera params.
  size_t numPixelsX = 320;
  size_t numPixelsY = 240;
  double focalLength = 0.03;
  double pixelSizeX = 0.001;
  double pixelSizeY = 0.002;
  double centerU = 100.0;
  double centerV = 125.0;
  CameraIntrinsicsPinhole<double> intrinsics(
    numPixelsX, numPixelsY, focalLength, pixelSizeX, pixelSizeY,
    centerU, centerV);

  // Get some data with which to test.
  std::vector< Vector3D<double> > points3D_world;
  std::vector< Vector3D<double> > points3D_camera;
  std::vector< Vector2D<double> > points2D;
  Transform3D<double> cameraTworld;
  this->get3DTestData(points3D_world, points3D_camera, cameraTworld);
  this->compute2DTestData(points2D, points3D_camera, intrinsics);

  // First test: recover just the camera params.
  Transform3DTo2D<double> projection0 =
    brick::computerVision::privateCode::estimateTransform3DTo2D<double>(
      points3D_camera.begin(), points3D_camera.end(), points2D.begin());
  for(unsigned int ii = 0; ii < points3D_camera.size(); ++ii) {
    Vector2D<double> projectedPoint = projection0 * points3D_camera[ii];
    double residual = magnitude<double>(projectedPoint - points2D[ii]);
    BRICK_TEST_ASSERT(approximatelyEqual(residual, 0.0, m_relaxedTolerance));
  }

  // Next test: recover projection comprised of rigid xf + camera params.
  Transform3DTo2D<double> projection1 =
    brick::computerVision::privateCode::estimateTransform3DTo2D<double>(
      points3D_world.begin(), points3D_world.end(), points2D.begin());
  for(unsigned int ii = 0; ii < points3D_world.size(); ++ii) {
    Vector2D<double> projectedPoint = projection1 * points3D_world[ii];
    double residual = magnitude<double>(projectedPoint - points2D[ii]);
    BRICK_TEST_ASSERT(approximatelyEqual(residual, 0.0, m_relaxedTolerance));
  }

}


void
CalibrationToolsTest::
testEstimateProjectedAreaAndCentroid()
{
  // Create a hypothetical calibration fiducial, off to the side of
  // the image, and rotated with respect to the camera.
  brick::numeric::Vector3D<double> circleCenter(3.0, 1.5, 1.0);
  brick::numeric::Vector3D<double> circleAxis0(0.2, 0.03, 0.02);
  brick::numeric::Vector3D<double> circleAxis1(0.02, 0.1, -0.05);
  brick::geometry::Circle3D<double> circle(
    circleCenter, circleAxis0, circleAxis1);

  // We'll need to know the plane in which the circle lies.
  brick::geometry::Plane3D<double> plane(
    circleCenter, circleCenter + circleAxis0, circleCenter + circleAxis1);

  // Create the hypothetical camera.
  CameraIntrinsicsPlumbBob<double> intrinsics;
  getTestIntrinsicsPlumbBob(intrinsics);

  // For independently estimating the centroid of the fiducial, we'll
  // want a higher resolution camera. Create that here.
  double const subsampleFactor = 5;
  CameraIntrinsicsPlumbBob<double> highResIntrinsics;
  getTestIntrinsicsPlumbBob(highResIntrinsics, subsampleFactor);

  // Pick a pixel range that completely surrounds the projection of
  // the circle.
  double rowMin = std::numeric_limits<double>::max();
  double rowMax = -rowMin;
  double columnMin = std::numeric_limits<double>::max();
  double columnMax = -columnMin;
  double angleIncrement = brick::common::constants::pi / 100;
  for(double angle = 0; angle < brick::common::constants::twoPi;
      angle += angleIncrement) {
    brick::numeric::Vector3D<double> perimeterPoint =
      circle.getPerimeterPoint(angle);
    brick::numeric::Vector2D<double> projectedPoint =
      highResIntrinsics.project(perimeterPoint);
    columnMin = std::min(columnMin, projectedPoint.getX());
    columnMax = std::max(columnMax, projectedPoint.getX());
    rowMin = std::min(rowMin, projectedPoint.getY());
    rowMax = std::max(rowMax, projectedPoint.getY());

    // Sanity check.
    brick::numeric::Vector3D<double> radiusVector =
      perimeterPoint - circleCenter;
    double measuredRadius = brick::numeric::magnitude<double>(radiusVector);
    BRICK_TEST_ASSERT(approximatelyEqual(measuredRadius, circle.getRadius(),
                                         m_defaultTolerance));

    // Second sanity check.
    double planeOffset = brick::numeric::dot<double>(
      radiusVector, plane.getNormal()) * measuredRadius;
    BRICK_TEST_ASSERT(planeOffset < m_defaultTolerance);
  }

  brick::common::Int32 startRow = std::floor(rowMin) - 5;
  brick::common::Int32 stopRow = std::ceil(rowMax) + 5;
  brick::common::Int32 startColumn = std::floor(columnMin) - 5;
  brick::common::Int32 stopColumn = std::ceil(columnMax) + 5;

  // Create an image of the circle.  Don't actually create it, but
  // compute the pixel values and accumulate the centroid of the
  // projection of the circle.  Actually, we will create the image for
  // debugging purposes...
  double referenceArea = 0.0;
  brick::numeric::Vector2D<double> centroidAccumulator(0.0, 0.0);
  Image<GRAY8> testImage(stopRow - startRow, stopColumn - startColumn);
  testImage = brick::common::UInt8(0);
  for(brick::common::Int32 row = startRow; row < stopRow; ++row) {
    for(brick::common::Int32 column = startColumn; column < stopColumn;
        ++column) {
      // Corners of pixels are at integer coordinates, so centers are
      // at "and-a-half" coordinates.
      brick::numeric::Vector2D<double> pixelCenter(column + 0.5, row + 0.5);

      // Figure out the point on the plane of the fiducial that
      // projects to this pixel.
      brick::geometry::Ray3D<double> ray =
        highResIntrinsics.reverseProject(pixelCenter);
      brick::numeric::Vector3D<double> intersect = findIntersect(ray, plane);

      // Sanity check.
      brick::numeric::Vector2D<double> reprojection =
        highResIntrinsics.project(intersect);
      double residual = brick::numeric::magnitude<double>(
        reprojection - pixelCenter);
      BRICK_TEST_ASSERT(residual < 1.0E-3);

      // Is the intersect within the circle?
      double radius = brick::numeric::magnitude<double>(
        intersect - circleCenter);
      if(radius <= circle.getRadius()) {
        centroidAccumulator += Vector2D<double>(column + 0.5, row + 0.5);
        referenceArea += 1.0;
        testImage(row - startRow, column - startColumn) =
          brick::common::UInt8(255);
      }
    }
  }
  brick::numeric::Vector2D<double> referenceCentroid =
    centroidAccumulator / referenceArea;
  referenceCentroid /= subsampleFactor;
  referenceArea /= (subsampleFactor * subsampleFactor);

  // Make a nice plot of centroid error.
  brick::common::UInt32 const smallestNumberOfTriangles = 80;
  brick::common::UInt32 const largestNumberOfTriangles = 81;
  brick::numeric::Array1D<double> projectionErrors(
    largestNumberOfTriangles - smallestNumberOfTriangles);
  brick::numeric::Array1D<double> areaErrors(
    largestNumberOfTriangles - smallestNumberOfTriangles);
  for(brick::common::UInt32 numberOfTriangles = smallestNumberOfTriangles;
      numberOfTriangles < largestNumberOfTriangles; ++numberOfTriangles) {

    // Recover the same quantities using the function under test.
    double area;
    brick::numeric::Vector2D<double> centroid;
    estimateProjectedAreaAndCentroid(area, centroid, circle, intrinsics,
                                     numberOfTriangles);

    // And remember errors.
    projectionErrors[numberOfTriangles - smallestNumberOfTriangles] =
      brick::numeric::magnitude<double>(centroid - referenceCentroid);
    areaErrors[numberOfTriangles - smallestNumberOfTriangles] =
      referenceArea - area;
  }

  // For comparison, see where the circle centroid projects to.
  brick::numeric::Vector2D<double> projectedCenter =
    intrinsics.project(circle.getOrigin());

  // Make sure this was actually a difficult test.
  double asymmetryMetric = brick::numeric::magnitude<double>(
    projectedCenter - referenceCentroid);
  BRICK_TEST_ASSERT(asymmetryMetric > 2.0);

  // Make sure our two different ways of computing the projected
  // centroid agree.
  BRICK_TEST_ASSERT(projectionErrors[0] < 0.01);

  // Make sure our two different ways of computing the projected
  // area agree.
  BRICK_TEST_ASSERT(areaErrors[0] < 0.2);
}


bool
CalibrationToolsTest::
checkIntrinsicsEqual(CameraIntrinsicsPinhole<double> const& intrinsics0,
                     CameraIntrinsicsPinhole<double> const& intrinsics1,
                     double tolerance)
{
  if(intrinsics0.getNumPixelsX() != intrinsics1.getNumPixelsX()) {
    return false;
  }
  if(intrinsics0.getNumPixelsY() != intrinsics1.getNumPixelsY()) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getKx(), intrinsics1.getKx(), tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getKy(), intrinsics1.getKy(), tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getCenterU(), intrinsics1.getCenterU(), tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getCenterV(), intrinsics1.getCenterV(), tolerance)) {
    return false;
  }
  return true;
}


bool
CalibrationToolsTest::
checkIntrinsicsEqual(CameraIntrinsicsPlumbBob<double> const& intrinsics0,
                     CameraIntrinsicsPlumbBob<double> const& intrinsics1,
                     double tolerance)
{
  if(intrinsics0.getNumPixelsX() != intrinsics1.getNumPixelsX()) {
    return false;
  }
  if(intrinsics0.getNumPixelsY() != intrinsics1.getNumPixelsY()) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getFocalLengthX(), intrinsics1.getFocalLengthX(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getFocalLengthY(), intrinsics1.getFocalLengthY(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getCenterU(), intrinsics1.getCenterU(), tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getCenterV(), intrinsics1.getCenterV(), tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getSkewCoefficient(), intrinsics1.getSkewCoefficient(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getRadialCoefficient0(), intrinsics1.getRadialCoefficient0(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getRadialCoefficient1(), intrinsics1.getRadialCoefficient1(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getRadialCoefficient2(), intrinsics1.getRadialCoefficient2(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getTangentialCoefficient0(),
       intrinsics1.getTangentialCoefficient0(),
       tolerance)) {
    return false;
  }
  if(!approximatelyEqual(
       intrinsics0.getTangentialCoefficient1(),
       intrinsics1.getTangentialCoefficient1(),
       tolerance)) {
    return false;
  }
  return true;
}


bool
CalibrationToolsTest::
checkTransformEqual(Transform3D<double> const& transform0,
                    Transform3D<double> const& transform1,
                    double tolerance)
{
  for(unsigned int ii = 0; ii < 4; ++ii) {
    for(unsigned int jj = 0; jj < 4; ++jj) {
      if(!approximatelyEqual(
           transform0(ii, jj), transform1(ii, jj), tolerance)) {
        return false;
      }
    }
  }
  return true;
}


bool
CalibrationToolsTest::
checkVectorEqual(Array1D<double> const& vector0,
                 Array1D<double> const& vector1,
                 double tolerance)
{
  if(vector0.size() != vector1.size()) {
    return false;
  }
  for(unsigned int ii = 0; ii < vector0.size(); ++ii) {
    if(!approximatelyEqual(vector0[ii], vector1[ii], tolerance)) {
      return false;
    }
  }
  return true;
}


template<class Intrinsics>
void
CalibrationToolsTest::
compute2DTestData(std::vector< Vector2D<double> >& points2D,
                  std::vector< Vector3D<double> >& points3D_camera,
                  Intrinsics const& intrinsics)
{
  // Generate corresponding 2D poinst.
  points2D.resize(points3D_camera.size());
  for(unsigned int ii = 0; ii < points3D_camera.size(); ++ii) {
    points2D[ii] = intrinsics.project(points3D_camera[ii]);
  }
}


void
CalibrationToolsTest::
get3DTestData(std::vector< Vector3D<double> >& points3D_world,
              std::vector< Vector3D<double> >& points3D_camera,
              Transform3D<double>& cameraTworld)
{
#if 1 /* Setting this to 0 makes for easier debugging. */
  cameraTworld = rollPitchYawToTransform3D(Vector3D<double>(0.1, -0.2, 0.3));
  cameraTworld.setValue<0, 3>(0.2);
  cameraTworld.setValue<1, 3>(-0.7);
  cameraTworld.setValue<2, 3>(-0.2);
#else
  cameraTworld.setValue(1.0, 0.0, 0.0, 0.0,
                        0.0, 1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 1.0);
#endif

  // Generate 3D points.
#if 1  /* Setting this to 0 makes for easier debugging of functions that */
       /* don't require many points.                                     */
  for(double zCoord = 1.0; zCoord < 10.0; zCoord += 2.4) {
    for(double yCoord = -1.0; yCoord < 1.0; yCoord += 0.2) {
      for(double xCoord = -1.0; xCoord < 1.0; xCoord += 0.31) {
#else
  for(double zCoord = 1.0; zCoord < 10.0; zCoord += 7.0) {
    for(double yCoord = -1.0; yCoord < 1.0; yCoord += 1.0) {
      for(double xCoord = -1.0; xCoord < 1.0; xCoord += 1.0) {
#endif
        points3D_world.push_back(Vector3D<double>(xCoord, yCoord, zCoord));
        points3D_camera.push_back(cameraTworld * (*(points3D_world.end() - 1)));
      }
    }
  }
}


void
CalibrationToolsTest::
getTestIntrinsicsPlumbBob(CameraIntrinsicsPlumbBob<double>& intrinsics,
                          double resolutionFactor)
{
  // Arbitrary camera params.
  size_t numPixelsX = 320 * resolutionFactor;
  size_t numPixelsY = 240 * resolutionFactor;
  double focalLengthX = 30.0 * resolutionFactor;
  double focalLengthY = 15.0 * resolutionFactor;
  double centerU = 100.0 * resolutionFactor;
  double centerV = 125.0 * resolutionFactor;
  double skewCoefficient = 0.001;
  double radialCoefficient0 = 0.02;
  double radialCoefficient1 = 0.0001;
  double radialCoefficient2 = 0.000007;
  double tangentialCoefficient0 = -0.01;
  double tangentialCoefficient1 = 0.005;
  CameraIntrinsicsPlumbBob<double> referenceIntrinsics(
    numPixelsX, numPixelsY, focalLengthX, focalLengthY, centerU, centerV,
    skewCoefficient, radialCoefficient0, radialCoefficient1, radialCoefficient2,
    tangentialCoefficient0, tangentialCoefficient1);
  intrinsics = referenceIntrinsics;
}


#if 0

int main(int argc, char** argv)
{
  CalibrationToolsTest currentTest;
  bool result = currentTest.run();
  return (result ? 0 : 1);
}

#else

namespace {

  CalibrationToolsTest currentTest;

}

#endif
