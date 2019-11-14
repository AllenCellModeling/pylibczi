#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <set>

#include "pb_helpers.h"
#include "Image.h"
#include "Reader.h"
#include "exceptions.h"

namespace pb_helpers {

  py::array packArray(pylibczi::ImageVector& images_)
  {
      // assumptions: The array contains images of the same size and the array is contiguous.
      images_.sort();
      auto charSizes = images_.getShape();

      unsigned long newSize = images_.front()->length()*images_.size();
      std::vector<ssize_t> shape(charSizes.size(), 0);
      std::transform(charSizes.begin(), charSizes.end(), shape.begin(), [](const std::pair<char, int>& a_) {
          return a_.second;
      });
      py::array* arrP = nullptr;
      switch (images_.front()->pixelType()) {
      case libCZI::PixelType::Gray8:
      case libCZI::PixelType::Bgr24: arrP = makeArray<uint8_t>(newSize, shape, images_);
          break;
      case libCZI::PixelType::Gray16:
      case libCZI::PixelType::Bgr48: arrP = makeArray<uint16_t>(newSize, shape, images_);
          break;
      case libCZI::PixelType::Gray32Float:
      case libCZI::PixelType::Bgr96Float: arrP = makeArray<float>(newSize, shape, images_);
          break;
      default: throw pylibczi::PixelTypeException(images_.front()->pixelType(), "Unsupported pixel type");
      }
      return *arrP;
  }

  py::array packStringArray(pylibczi::SubblockMetaVec& metadata_){
      metadata_.sort();
      auto charSizes = metadata_.getShape();

      std::vector<ssize_t> shape(charSizes.size(), 0);
      std::transform(charSizes.begin(), charSizes.end(), shape.begin(), [](const std::pair<char, int>& a_) {
          return a_.second;
      });
      return makeStrArray(metadata_, shape);
  }

  py::array makeStrArray(pylibczi::SubblockMetaVec& metadata_, std::vector<ssize_t>& shape_){
      std::vector<std::string> ans(metadata_.size());
      std::transform(metadata_.begin(), metadata_.end(), ans.begin(), [](const pylibczi::SubblockString& a_){
          return a_.getString();
      });
      return py::array(py::cast(ans), shape_);
  }


}
