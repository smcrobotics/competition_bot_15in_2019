#include "main.h"

#include "smc/robot.h"
#include "smc/tasks.h"
#include "smc/util/Binding.h"

using namespace okapi;
using std::cout;
using std::endl;

typedef okapi::ControllerButton Button;

/// Begin forward declaration block
std::shared_ptr<okapi::AsyncMotionProfileController> robot::profile_controller;
std::shared_ptr<okapi::ChassisControllerIntegrated> robot::chassis;
/// End forward declaration block


/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button() {
    static bool pressed = false;
    pressed = !pressed;
    if (pressed) {
        pros::lcd::set_text(2, "I was pressed!");
    } else {
        pros::lcd::clear_line(2);
    }
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
    robot::chassis = ChassisControllerFactory::createPtr(
            okapi::MotorGroup{robot::BACK_LEFT_DRIVE_MOTOR_PORT, robot::FRONT_LEFT_DRIVE_MOTOR_PORT},
            okapi::MotorGroup{robot::BACK_RIGHT_DRIVE_MOTOR_PORT, robot::FRONT_RIGHT_DRIVE_MOTOR_PORT},
            AbstractMotor::gearset::green, {2_in, 12_in}
    );
    robot::profile_controller = std::make_shared<AsyncMotionProfileController>(
            TimeUtilFactory::create(),
            1.0, 0.5, 1.5,
            robot::chassis->getChassisModel(),
            robot::chassis->getChassisScales(),
            robot::chassis->getGearsetRatioPair()
    );

    auto motionProfiler = AsyncControllerFactory


//    robot::chassis->setBrakeMode(okapi::Motor::brakeMode::brake);
    robot::profile_controller->generatePath({
        Point{0_ft, 0_ft, 0_deg},
        Point{12_in, 12_in, 0_deg}},
                "A" // Profile name
    );
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */


void autonomous() {
//    int timeout = 10;
//    pros::Task myTask(intake_task_fn, (void*) &timeout, "My Task");kd

    robot::chassis->setBrakeMode(constants::OKAPI_BRAKE);
    robot::profile_controller->setTarget("A");
    robot::profile_controller->waitUntilSettled();
    robot::chassis->setBrakeMode(constants::OKAPI_COAST);
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void opcontrol() {
    okapi::Controller master(okapi::ControllerId::master);
    intake::init();

    double intakeVel = 0;
    bool isBrake = false;
    std::vector<Binding> bind_list;

    /** Begin bind block **/
    // Brake toggle binding
    bind_list.emplace_back(Binding(okapi::ControllerButton(bindings::DRIVE_BRAKE_TOGGLE), nullptr,
            [isBrake, master]() mutable {
        isBrake = !isBrake;
        robot::chassis->setBrakeMode(isBrake? constants::OKAPI_BRAKE : constants::OKAPI_COAST);
        master.setText(0, 0, isBrake ? "Brake mode on " : "Brake mode off");
    }, nullptr));

    // Intake hold binding
    bind_list.emplace_back(Binding(okapi::ControllerButton(bindings::INTAKE_BUTTON), []() {
        intake::setIntakeVelocity(100);
    }, []() {
        intake::setIntakeVelocity(0);
    }, nullptr));

    // Outtake hold binding
    bind_list.emplace_back(Binding(okapi::ControllerButton(bindings::OUTTAKE_BUTTON), []() {
        intake::setIntakeVelocity(-100);
    }, []() {
        intake::setIntakeVelocity(0);
    }, nullptr));

    // Arm position bindings
    bind_list.emplace_back(Binding(Button(bindings::INTAKE_POS_UP), []() {
        intake::moveArmsToPosition(intake::Position::UP);
    }, nullptr, nullptr));
    bind_list.emplace_back(Binding(Button(bindings::INTAKE_POS_DOWN), []() {
        intake::moveArmsToPosition(intake::Position::DOWN);
    }, nullptr, nullptr));

    // TODO: Remove this before competition
    bind_list.emplace_back(Binding(okapi::ControllerButton(okapi::ControllerDigital::Y), autonomous, nullptr, nullptr)); // Bind for auto test
    // Note: Auto bind is blocking
    /** End bind block **/
    
    cout << "hey we here" << endl;
#if (AUTO_DEBUG == 1)
    bool should_continue = true;
    while (should_continue) {
        if (master.getDigital(pros::E_CONTROLLER_DIGITAL_A))
            autonomous();
        else if (master.getDigital(pros::E_CONTROLLER_DIGITAL_B))
            should_continue = false;
    }
#else
    while (true) {
        drive::opControl(master);
        
        for (Binding b : bind_list)
            b.update();
        intake::printPos();


        pros::delay(2);
    }
#endif
}
#pragma clang diagnostic pop