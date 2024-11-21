#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include <thread>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <iostream>

class KeyboardToJoystick : public rclcpp::Node
{
public:
    KeyboardToJoystick() : Node("keyboard_to_joystick"), is_publishing_(true)
    {
        // Create a publisher for Joy messages
        joy_publisher_ = this->create_publisher<sensor_msgs::msg::Joy>("joy", 10);

        // Initialize the Joy message
        joy_msg_.axes.resize(8, 0.0);  // Adjust size as needed
        joy_msg_.buttons.resize(13, 0);  // Adjust size as needed

	tcgetattr(STDIN_FILENO, &orig_termios_);
    	struct termios new_termios = orig_termios_;
    	new_termios.c_lflag &= ~(ICANON | ECHO);
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        // Create a timer to periodically publish Joy messages
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&KeyboardToJoystick::publishJoyMessage, this));

        // Start a thread to listen for keyboard input
        keyboard_thread_ = std::thread(&KeyboardToJoystick::keyboardListener, this);
    }

    ~KeyboardToJoystick()
    {
        // Stop the keyboard thread
        keep_running_ = false;
        if (keyboard_thread_.joinable())
        {
            keyboard_thread_.join();
        }
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
	RCLCPP_INFO(this->get_logger(), "Node shutdown!");
    }

private:
    void publishJoyMessage()
    {
        if (is_publishing_)
        {
            // Update the header timestamp
            joy_msg_.header.stamp = this->get_clock()->now();

            // Publish the message
            joy_publisher_->publish(joy_msg_);
        }
    }

    void keyboardListener()
    {
        // Disable terminal echo for raw input
        // struct termios oldt, newt;
        // tcgetattr(STDIN_FILENO, &oldt);
        // newt = oldt;
        // newt.c_lflag &= ~(ICANON | ECHO);
        // tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        char ch;
        while (keep_running_)
        {
            ch = getchar();  // Get a character from the terminal
            if (ch == ' ')   // Space key stops publishing
            {
                is_publishing_ = false;
                RCLCPP_INFO(this->get_logger(), "Stopped publishing Joy message, Car will be running.");
            }
            else if (ch == 's') // 's' key starts publishing
            {
                is_publishing_ = true;
                RCLCPP_INFO(this->get_logger(), "Resumed publishing Joy message, Car should stop.");
            }
            else if (ch == 'q')
            {
            	RCLCPP_INFO(this->get_logger(), "q pressed!Press Enter to return to Terminal.");
            	rclcpp::shutdown();
            }
        }

        // Restore terminal settings
        // tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }

    rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    sensor_msgs::msg::Joy joy_msg_;
    std::thread keyboard_thread_;
    std::atomic<bool> is_publishing_;
    std::atomic<bool> keep_running_{true};
    struct termios orig_termios_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<KeyboardToJoystick>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}

