/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <iostream>
#include <memory>
#include "src/common/log_adapter.h"
#include "common/common_test.h"
#include "mindspore/lite/src/common/file_utils.h"
#include "mindspore/lite/src/runtime/opencl/opencl_runtime.h"
#include "mindspore/lite/src/runtime/kernel/opencl/subgraph_opencl_kernel.h"
#include "mindspore/lite/src/runtime/kernel/opencl/kernel/reshape.h"
#include "mindspore/lite/test/ut/src/runtime/kernel/opencl/utils_tests.h"

namespace mindspore {
class TestReshapeOpenCL : public mindspore::CommonTest {
 public:
  TestReshapeOpenCL() {}
};

void RunTestCaseReshape(const std::vector<int> &shape_in, const std::vector<int> &shape_out, void *input_data,
                        void *output_data, bool enable_fp16) {
  auto ocl_runtime = lite::opencl::OpenCLRuntimeWrapper().GetInstance();
  ocl_runtime->Init();
  size_t dtype_size = enable_fp16 ? sizeof(float16_t) : sizeof(float);
  ocl_runtime->SetFp16Enable(enable_fp16);
  auto allocator = ocl_runtime->GetAllocator();
  auto tensor_x_ptr = std::make_unique<lite::Tensor>(TypeId(enable_fp16 ? kNumberTypeFloat16 : kNumberTypeFloat32),
                                                     shape_in, schema::Format_NHWC);
  auto tensor_x = tensor_x_ptr.get();
  if (tensor_x == nullptr) {
    MS_LOG(ERROR) << "tensor_x create error.";
    return;
  }
  bool is_output_2d = shape_out.size() == 2;
  auto tensor_out_ptr =
    std::make_unique<lite::Tensor>(TypeId(enable_fp16 ? kNumberTypeFloat16 : kNumberTypeFloat32), shape_out,
                                   is_output_2d ? schema::Format_NC : schema::Format_NHWC);
  auto tensor_out = tensor_out_ptr.get();
  if (tensor_out == nullptr) {
    MS_LOG(ERROR) << "tensor_out create error.";
    return;
  }
  std::vector<lite::Tensor *> inputs{tensor_x};
  std::vector<lite::Tensor *> outputs{tensor_out};
  auto arith_kernel_ptr = std::make_unique<kernel::ReshapeOpenCLKernel>(nullptr, inputs, outputs);
  auto arith_kernel = arith_kernel_ptr.release();
  if (arith_kernel == nullptr) {
    MS_LOG(ERROR) << "arith_kernel create error.";
    return;
  }
  arith_kernel->Init();

  inputs[0]->MallocData(allocator);

  std::vector<kernel::LiteKernel *> kernels{arith_kernel};
  auto pGraph_ptr = std::make_unique<kernel::SubGraphOpenCLKernel>(inputs, outputs, kernels, kernels, kernels);
  auto pGraph = pGraph_ptr.get();
  if (pGraph == nullptr) {
    MS_LOG(ERROR) << "pGraph create error.";
    return;
  }
  pGraph->Init();
  memcpy(inputs[0]->MutableData(), input_data, inputs[0]->ElementsNum() * dtype_size);
  pGraph->Run();

  if (enable_fp16) {
    CompareOutput(outputs[0]->MutableData(), output_data, outputs[0]->ElementsNum(), static_cast<float16_t>(1e-3),
                  2e-2);
  } else {
    CompareOutput(outputs[0]->MutableData(), output_data, outputs[0]->ElementsNum(), static_cast<float>(1e-5));
  }
  for (auto t : inputs) {
    t->SetData(nullptr);
  }
  for (auto t : outputs) {
    t->SetData(nullptr);
  }

  MS_LOG(INFO) << "Test Reshape passed";
}

TEST_F(TestReshapeOpenCL, ReshapeFp32) {
  std::vector<int> shape_in = {1, 1, 1, 7};
  std::vector<int> shape_out = {1, 7};
  std::vector<float> input_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float> output_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}

TEST_F(TestReshapeOpenCL, ReshapeFp16) {
  std::vector<int> shape_in = {1, 1, 1, 7};
  std::vector<int> shape_out = {1, 7};
  std::vector<float16_t> input_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
  std::vector<float16_t> output_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), true);
}

TEST_F(TestReshapeOpenCL, Reshape4DFp32) {
  std::vector<int> shape_in = {1, 2, 2, 3};
  std::vector<int> shape_out = {1, 1, 4, 3};
  std::vector<float> input_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f};
  std::vector<float> output_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}

TEST_F(TestReshapeOpenCL, Reshape4DFp16) {
  std::vector<int> shape_in = {1, 2, 2, 3};
  std::vector<int> shape_out = {1, 1, 4, 3};
  std::vector<float16_t> input_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f};
  std::vector<float16_t> output_data = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), true);
}

TEST_F(TestReshapeOpenCL, Reshape4D2DFp32) {
  std::vector<int> shape_in = {1, 2, 2, 4};
  std::vector<int> shape_out = {4, 4};
  std::vector<float> input_data = {0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
                                   8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};
  std::vector<float> output_data = {0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
                                    8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}
TEST_F(TestReshapeOpenCL, Reshape4DFp32Rem10) {
  std::vector<int> shape_in = {1, 3, 2, 4};
  std::vector<int> shape_out = {1, 4, 2, 3};
  std::vector<float> input_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f,
                                   12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f};
  std::vector<float> output_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f,
                                    12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}
TEST_F(TestReshapeOpenCL, Reshape4DFp32Rem01Test0) {
  std::vector<int> shape_in = {1, 4, 2, 3};
  std::vector<int> shape_out = {1, 3, 2, 4};
  std::vector<float> input_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f,
                                   12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f};
  std::vector<float> output_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f,
                                    12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}
TEST_F(TestReshapeOpenCL, Reshape4DFp32Rem01Test1) {
  std::vector<int> shape_in = {1, 2, 2, 5};
  std::vector<int> shape_out = {1, 1, 5, 4};
  std::vector<float> input_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,
                                   10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f};
  std::vector<float> output_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,
                                    10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}
TEST_F(TestReshapeOpenCL, Reshape4DFp32Rem01Test2) {
  std::vector<int> shape_in = {1, 4, 2, 5};
  std::vector<int> shape_out = {1, 2, 5, 4};
  std::vector<float> input_data = {
    0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f, 12.0f, 13.0f,
    14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f,
    28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f,
  };
  std::vector<float> output_data = {
    0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,  10.0f, 11.0f, 12.0f, 13.0f,
    14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f,
    28.0f, 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f,
  };

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}
TEST_F(TestReshapeOpenCL, Reshape4DFp32Rem11) {
  std::vector<int> shape_in = {1, 3, 2, 5};
  std::vector<int> shape_out = {1, 5, 2, 3};
  std::vector<float> input_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,
                                   10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f,
                                   20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f};
  std::vector<float> output_data = {0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,  9.0f,
                                    10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f,
                                    20.0f, 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f};

  RunTestCaseReshape(shape_in, shape_out, input_data.data(), output_data.data(), false);
}

}  // namespace mindspore
