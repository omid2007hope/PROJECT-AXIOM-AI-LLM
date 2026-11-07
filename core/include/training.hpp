#ifndef include_training
#define include_training

#include <string>
#include <vector>

#include "../include/config.hpp"
#include "../include/cpu.hpp"
#include "../include/gpu.hpp"

// What one AI::train() call did
enum class TrainStatus {
	Trained, // one full forward + backward step ran
	NoData,	 // the RAM buffer is exhausted: the caller must refill it
	Failed	 // a CUDA step failed (already reported on stderr)
};

struct TrainingTelemetry {
	double mean_loss = 0.0;
	double gradient_norm = 0.0;
	double activation_magnitude = 0.0;
	double dead_fraction = 0.0;
	size_t nonfinite_count = 0;
	size_t steps = 0;
};

class Training {
public:
	// The Training Loop Orchestrator
	TrainStatus train(Optimizer& optimizer, Portion& portion);

	// Mean training loss (0.5 * squared error per step, measured before each update)
	// over the steps since the previous call. `steps` receives how many that was.
	// Waits for the GPU to finish, so call it once per report interval, not per step.
	bool read_loss(double& mean_loss, size_t& steps);

	bool read_telemetry(TrainingTelemetry& telemetry);

	bool evaluate_validation(const std::vector<uint32_t>& validation_tokens,
				 size_t window_size,
				 double& validation_loss);

	// Weight persistence methods
	bool save_weights(const std::string& path = "weights.axiom");
};
#endif // include_training S