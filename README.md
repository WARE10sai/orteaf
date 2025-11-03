# ORTEAF
ORTEAF(Orchestrated Tensor Execution Adapter Framework) is an orchestration framework that unifies multiple compute APIs (CUDA/MPS/CPU) under a common tensor layer.

## 🧠 Philosophy

ORTEAF is designed primarily to support **Spiking Neural Networks (SNNs)**, enabling efficient learning and inference even on edge devices.  
The goal is to achieve high-speed, low-power computation while maintaining flexibility across heterogeneous hardware.

## 🔍 Motivation

To realize the above philosophy, ORTEAF implements a custom learning algorithm called **JBB (Jacobian-Based Backpropagation)**.  
This framework serves as the backbone for the research and engineering required to bring JBB into practical use.

## 🚀 Key Features

- **Unified Tensor Layer:**  
  Provides a consistent interface across CUDA, MPS, and CPU runtimes.
- **JBB Integration:**  
  Natively supports the JBB (Jacobian-Based Backpropagation) algorithm.
- **SNN Optimization:**  
  Designed for high-speed inference and learning on resource-constrained devices.
- **PyTorch-like API:**  
  Easy to learn and use; familiar structure for PyTorch users.
- **Modular & Extensible:**  
  Clean dependency graph and loosely coupled modules make contribution easy.

貢献ルールはcontributing.mdを見てください。

## ⚙️ Installation

### Using CMake with Ninja
```bash
git clone https://github.com/yourname/orteaf.git
cd orteaf
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### Enabling or Disabling CUDA and MPS

You can control which backends are enabled at build time through CMake options.

```bash
cmake -G Ninja .. -DENABLE_CUDA=ON -DENABLE_MPS=OFF
```

| Option | Description | Default |
|---------|-------------|----------|
| `ENABLE_CUDA` | Enable CUDA backend | ON |
| `ENABLE_MPS` | Enable Metal (MPS) backend | ON |

If both are disabled, the build will default to the CPU runtime.

## 📜 License
ORTEAF is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.