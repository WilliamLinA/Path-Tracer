#ifndef MLT_SAMPLER_H
#define MLT_SAMPLER_H

#include "rtweekend.h"
#include <vector>
#include <utility>

// Kelemen-style MLT sampler for Primary Sample Space MLT
// Based on "A Simple and Robust Mutation Strategy for the MLT Algorithm" (Kelemen et al. 2002)
class MLTSampler {
private:
    struct SampleRecord {
        double value;          // Current value in [0,1)
        size_t last_modified;  // Last iteration when this sample was modified
        
        SampleRecord(double v) : value(v), last_modified(0) {}
    };
    
    std::vector<SampleRecord> m_samples;  // Primary sample space vector
    std::vector<std::pair<size_t, SampleRecord>> m_backup;  // For reject()
    
    size_t m_time;           // Current iteration number
    size_t m_large_step_time;  // Last large step time
    bool m_large_step;       // Is current mutation a large step?
    size_t m_sample_index;   // Current sample dimension being accessed
    
    // Mutation parameters (Kelemen et al. defaults)
    double m_s1;  // Small mutation size: 1/1024
    double m_s2;  // Large mutation size: 1/64
    double m_log_ratio;  // Precomputed: -log(s2/s1)
    
public:
    MLTSampler() 
        : m_time(0), m_large_step_time(0), m_large_step(false), 
          m_sample_index(0), m_s1(1.0/1024.0), m_s2(1.0/64.0) {
        m_log_ratio = -log(m_s2 / m_s1);
    }
    
    // Get next random sample (possibly mutated)
    double next() {
        return primary_sample(m_sample_index++);
    }
    
    // Reset sample index for new path generation
    void start_iteration() {
        m_sample_index = 0;
    }
    
    // Set whether current mutation is a large step
    void set_large_step(bool large) {
        m_large_step = large;
    }
    
    bool is_large_step() const {
        return m_large_step;
    }
    
    // Accept the current mutation
    void accept() {
        if (m_large_step)
            m_large_step_time = m_time;
        m_time++;
        m_backup.clear();
    }
    
    // Reject the current mutation (restore from backup)
    void reject() {
        for (const auto& backup : m_backup) {
            m_samples[backup.first] = backup.second;
        }
        m_backup.clear();
    }
    
    // Reset sampler state (for new chain)
    void reset() {
        m_time = 0;
        m_large_step_time = 0;
        m_sample_index = 0;
        m_samples.clear();
        m_backup.clear();
    }
    
private:
    // Core: get primary sample with lazy evaluation and mutation
    double primary_sample(size_t i) {
        // Expand sample vector if needed
        while (i >= m_samples.size()) {
            m_samples.push_back(SampleRecord(random_double()));
        }
        
        // If this sample hasn't been touched this iteration, mutate it
        if (m_samples[i].last_modified < m_time) {
            if (m_large_step) {
                // Large step: completely replace with new random value
                m_backup.push_back(std::make_pair(i, m_samples[i]));
                m_samples[i].value = random_double();
                m_samples[i].last_modified = m_time;
            } else {
                // Small step: apply Kelemen mutation
                // First, check if we need to reset to large step time
                if (m_samples[i].last_modified < m_large_step_time) {
                    m_samples[i].value = random_double();
                    m_samples[i].last_modified = m_large_step_time;
                }
                
                // Apply small mutations for all skipped iterations
                while (m_samples[i].last_modified + 1 < m_time) {
                    m_samples[i].value = mutate(m_samples[i].value);
                    m_samples[i].last_modified++;
                }
                
                // Backup and apply final mutation
                m_backup.push_back(std::make_pair(i, m_samples[i]));
                m_samples[i].value = mutate(m_samples[i].value);
                m_samples[i].last_modified++;
            }
        }
        
        return m_samples[i].value;
    }
    
    // Kelemen-style mutation: perturb value in [0,1)
    double mutate(double value) {
        double sample = random_double();
        bool add = (sample < 0.5);
        sample = add ? (sample * 2.0) : ((sample - 0.5) * 2.0);
        
        // Exponential distribution for mutation size
        double dv = m_s2 * exp(sample * m_log_ratio);
        
        if (add) {
            value += dv;
            if (value >= 1.0) value -= 1.0;
        } else {
            value -= dv;
            if (value < 0.0) value += 1.0;
        }
        
        return value;
    }
};

#endif // MLT_SAMPLER_H
