#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <random>
#include <typeinfo>

#include <lqr_steer_control.h>
#include <pid_steer_control.h>
#include <curvature_steer_control.h>
#include <position_filter.h>

#define TRACK_POINT_SPLIT_MAX 256
#define TRACK_POINT_SPLIT_HALF 128

int splined_points_cursor = 0;
double time_sum = 1;

position_filter pos_filter;

std::vector<std::string> splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<path_point_t> get_path(std::string file_name, int length, int cursor)
{
    std::ifstream inputFile(file_name);
    std::string line;
    std::vector<path_point_t> result;
    static path_point_t origin = {0, 0, 0, 0, 0};
    int flag = 0;

    if (!inputFile.is_open()) {
        std::cerr << "can not open." << std::endl;
        return std::vector<path_point_t>();
    }
    int line_num = 0;
    while (std::getline(inputFile, line)) {
        if (line_num++ < cursor) {
            continue;
        }
        std::vector<std::string> splited_line = splitString(line, ',');
        path_point splined_point = {std::stod(splited_line[0]), std::stod(splited_line[1]), std::stod(splited_line[2]), std::stod(splited_line[4]), std::stod(splited_line[3])};
        // if (cursor == 0 && flag == 0) {
        //     origin = splined_point;
        //     flag = 1;
        // }
        // splined_point.x -= origin.x;
        // splined_point.y -= origin.y;
        result.push_back(splined_point);
        if ((line_num - cursor + 1) >= length) {
            break;
        }
    }
    inputFile.close();
    return result;
}

void predict_next_velocity_steer(double& velocity, double& steer, double target_velocity, double target_steer, double dt)
{
    const double max_steer_velocity = 20.0 * PT_M_PI / 180.0;
    const double max_accel = 0.8333333;
    const double threshold_steer_diff_angle = 3 * PT_M_PI / 180.0;
    const int max_steer_error_level = 10;
    const double max_steer_angle = 45.0 * PT_M_PI / 180.0;
    const double max_speed = 10.0 / 3.6;
    const double min_speed = 1;

    double max_steer_change_amount = max_steer_velocity * dt;
    double dsteer = target_steer - steer;
    double revise_target_steer = steer;
    double calculated_velocity = 0;
    if (dsteer > max_steer_change_amount) {
        revise_target_steer += max_steer_change_amount;
    } else if (dsteer < -max_steer_change_amount) {
        revise_target_steer -= max_steer_change_amount;
    } else {
        revise_target_steer = target_steer;
    }

    if (revise_target_steer > max_steer_angle) {
        revise_target_steer = max_steer_angle;
    } else if (revise_target_steer < -max_steer_angle) {
        revise_target_steer = -max_steer_angle;
    }

    int velocity_control_level = (int)(fabsf(dsteer) / threshold_steer_diff_angle);
    calculated_velocity = target_velocity - (target_velocity - min_speed) * ((double)velocity_control_level / max_steer_error_level);

    if (calculated_velocity > max_speed) {
        calculated_velocity = max_speed;
    } else if (calculated_velocity < min_speed) {
        calculated_velocity = min_speed;
    }

    double max_velocity_change_amount = max_accel * dt;
    double d_velocity = calculated_velocity - velocity;
    double predict_velocity = velocity;
    if (d_velocity > max_velocity_change_amount) {
        predict_velocity += max_velocity_change_amount;
    } else if (d_velocity < -max_velocity_change_amount) {
        predict_velocity -= max_velocity_change_amount;
    } else {
        predict_velocity  = calculated_velocity;
    }

    steer = revise_target_steer;
    velocity = predict_velocity;

}


int main(int argc, const char * argv[])
{
    // path_tracker* tracker;
    curvature_steer_control tracker(PT_M_PI_2/2, 2.5, 2.15, 0);
    pt_control_state_t current_state;
    double dt = 0.05;

    // pt_control_state_t init = {0, 0, PT_M_PI_2 / 2, 0.15, 0};
    // pt_control_state_t past = init;
    // double dt = 0.001;
    // double target_v = 1;
    // double a = 0.83333;
    // double moving_distance = 0;
    // for (int i = 0; i < 10000; i++) {
    //     init = tracker.update_predict_state(init, dt);
    //     init.v += a * dt;
    //     if (init.v > target_v) {
    //         init.v = target_v;
    //     }
    //     moving_distance += std::sqrt(std::pow(init.x - past.x, 2) + std::pow(init.y - past.y, 2));
    //     past = init;
    // }
    // printf("%lf %lf %lf %lf\n", init.x, init.y, init.yaw, moving_distance);
    // return 0;

    std::string log_name = "log.csv";
    // std::string map_file_path = "../../../path_gps_smi_p_final.csv";path_new_map   path_smi_new_mrp2000_7km   path_smi_new_mrp2000_7km path_debug
    // std::string map_file_path = "D:\\git\\git_luxrobo\\golfcart_vehicle_control_unit_stm32\\application\\User\\lib\\golfcar_lqr_path_manager\\path_debug.csv";
    std::string map_file_path = "D:\\git\\git_luxrobo\\golfcart_vehicle_control_unit_stm32\\application\\User\\lib\\golfcar_lqr_path_manager\\path_cl.csv";
    // std::string map_file_path = "D:\\git\\git_luxrobo\\golfcart_vehicle_control_unit_stm32\\application\\User\\lib\\golfcar_lqr_path_manager\\path_new_map.csv";

    std::ofstream outputFile(log_name);
    int count = 1;
    while (!outputFile.is_open()) {
        outputFile.close();

        log_name.clear();

        std::ostringstream oss;
        oss << count++ << "log.csv";
        log_name = oss.str();
        outputFile = std::ofstream(log_name);
    }
    int end_flag = 0;

    std::vector<path_point_t> splined_points_size_cutting = get_path(map_file_path, TRACK_POINT_SPLIT_MAX, splined_points_cursor);

    if (splined_points_size_cutting.size() < TRACK_POINT_SPLIT_MAX) {
        splined_points_cursor += splined_points_size_cutting.size();
        end_flag = 1;
    } else {
        splined_points_cursor += TRACK_POINT_SPLIT_HALF;
    }
    // path_point_t origin = {splined_points_size_cutting[0].x, splined_points_size_cutting[0].y, 0,0,0};
    pt_control_state_t init_state = {splined_points_size_cutting[0].x, splined_points_size_cutting[0].y, splined_points_size_cutting[0].yaw, 0, 0};
    tracker.set_path_points(init_state, splined_points_size_cutting);

    current_state = init_state;
    double filter_init_pos[3] = {init_state.x, init_state.y, init_state.yaw};
    pos_filter.set_xy(ModelMatrix_D(3, 1, filter_init_pos));

    while (true) {
        if (tracker.get_remain_point_num() < TRACK_POINT_SPLIT_HALF && (end_flag == 0)) {
            splined_points_size_cutting = get_path(map_file_path, TRACK_POINT_SPLIT_MAX, splined_points_cursor);
            if (splined_points_size_cutting.size() < TRACK_POINT_SPLIT_MAX) { // 남은 개수가 절반보다 남지 않은 경우
                splined_points_cursor = 0;
                end_flag = 1;
            } else {
                splined_points_cursor += TRACK_POINT_SPLIT_HALF;
            }
            tracker.set_path_points(tracker.get_state(), splined_points_size_cutting);
        }

        double temp_input[3] = {current_state.v, current_state.steer, dt};
        pos_filter.predict(ModelMatrix_D(3, 1, temp_input));

        pt_control_state_t predict_state = {pos_filter.get_xy().get(0, 0),
                                               pos_filter.get_xy().get(1, 0),
                                               pos_filter.get_xy().get(2, 0),
                                               current_state.steer,
                                               current_state.v};
        tracker.set_state(predict_state, 0);

        pt_update_result_t update_result = tracker.update(dt);

        if (update_result != PT_UPDATE_RESULT_RUNNING) {
            if (update_result == PT_UPDATE_RESULT_NOT_READY) {
                continue;
            }
            printf("finish %d", (int)update_result);
            return 0;
        } else {
            static int debug_count = 0;
            predict_next_velocity_steer(current_state.v, current_state.steer, tracker.get_target_velocity(), tracker.get_target_steer(), dt);

            // if (debug_count % 10 == 0) {
                outputFile = std::ofstream(log_name, std::ios::app);

                // 파일이 열렸는지 확인
                if (!outputFile.is_open()) {
                    std::cerr << "can not open." << std::endl;
                    return 1;
                }

                pt_control_state_t tracker_state = tracker.get_state();
                char debug_string[200];
                int index = tracker.get_front_target_point_index();
                int real_index = tracker.get_target_point_index();
                sprintf(debug_string, "%lf,%lf,%lf,%lf,%lf,%lf,,%lf,%lf,%lf,,%lf,%lf,%lf",tracker_state.x,
                                                                    tracker_state.y,
                                                                    tracker_state.yaw,
                                                                    tracker_state.steer,
                                                                    tracker.get_yaw_error(),
                                                                    tracker_state.v,
                                                                    splined_points_size_cutting[index].x,
                                                                    splined_points_size_cutting[index].y,
                                                                    splined_points_size_cutting[index].yaw,
                                                                    splined_points_size_cutting[real_index].x,
                                                                    splined_points_size_cutting[real_index].y,
                                                                    splined_points_size_cutting[real_index].yaw);
                outputFile << debug_string << "\n";
                // std::cout << debug_string << std::endl;
                outputFile.close();
            // }
            debug_count++;
        }
    }


    return 0;
}

