/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include "interrupts.hpp"

int main(int argc, char** argv) {

    // vectors is a C++ std::vector of strings that contain the address of the ISR
    // delays  is a C++ std::vector of ints that contain the delays of each device
    // the index of these elements is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;      //!< string to store single line of trace file
    std::string execution;  //!< string to accumulate the execution output

    /******************ADD YOUR VARIABLES HERE*************************/
    int current_time = 0;
    const int default_context_save = 10;
    const int isr_activity = 40;
    /******************************************************************/

    // parse each line of the input trace file
    while (std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        /******************ADD YOUR SIMULATION CODE HERE*************************/
        if (trace.size() == 0) continue;
        if (activity == "null" || duration_intr < 0) {
            continue;
        }

        for (auto &c : activity) {
            c = std::toupper(c);
        }

        if (activity.find("CPU") != std::string::npos) {
            int dur = duration_intr;
            execution += std::to_string(current_time) + ", " + std::to_string(dur) + ", CPU Burst\n";
            current_time += dur;
        }
        else if (activity.find("SYSCALL") != std::string::npos ||
                 activity.find("END_IO") != std::string::npos ||
                 activity.find("ENDIO") != std::string::npos) {
            
            int device_num = duration_intr;
            if (device_num < 0 || device_num >= (int)delays.size() || device_num >= (int)vectors.size()) {
                execution += std::to_string(current_time) + ", 1, invalid device " + std::to_string(device_num) + "\n";
                continue;
            }

            // 1) standard interrupt boilerplate: switch to kernel, save context, find vector, load address
            auto [boiler, new_time] = intr_boilerplate(current_time, device_num, default_context_save, vectors);
            execution += boiler;
            current_time = new_time;

            // 2) execute ISR body: the total ISR body duration should equal the device delay
            int total_delay = delays.at(device_num); // from device_table.txt
            int remaining = total_delay;

            // Choose a textual label depending on activity (SYSCALL vs END_IO)
            std::string isr_label;
            if (activity.find("SYSCALL") != std::string::npos)
                isr_label = "SYSCALL: run the ISR (device driver)";
            else
                isr_label = "ENDIO: run the ISR (device driver)";

            // We'll break the ISR into chunks of 'isr_activity' ms (except last remainder).
            while (remaining > 0) {
                int chunk = std::min(remaining, isr_activity);
                execution += std::to_string(current_time) + ", " + std::to_string(chunk) + ", " + isr_label + "\n";
                current_time += chunk;
                remaining -= chunk;
            }

            // 3) IRET (1ms)
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        }
        else {
            // Unknown activity — log and skip
            execution += std::to_string(current_time) + ", 1, UNKNOWN ACTIVITY: " + activity + "\n";
        }
        /************************************************************************/
    }

    input_file.close();

    write_output(execution);

    return 0;
}
