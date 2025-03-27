#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

// ---------------------------------------------
// ماژول سنسور صدا
// ---------------------------------------------
SC_MODULE(SoundSensor) {
    tlm_utils::simple_initiator_socket<SoundSensor> socket;
    SC_CTOR(SoundSensor) {
        SC_THREAD(generate_sound);
    }

    void generate_sound() {
        wait(10, SC_NS);
        send_sound(1);  // پالس اول

        wait(100, SC_NS);
        send_sound(1);  // پالس دوم (مشابه تلفن)

        wait(100, SC_NS);
        send_sound(1);  // پالس سوم (مشابه تلفن)

        wait(500, SC_NS);
        send_sound(1);  // یک پالس جدا (مشابه در)
    }

    void send_sound(int signal) {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&signal));
        trans.set_data_length(sizeof(signal));

        socket->b_transport(trans, delay);
    }
};

// ---------------------------------------------
// ماژول پردازشگر صدا
// ---------------------------------------------
SC_MODULE(SoundProcessor) {
    tlm_utils::simple_target_socket<SoundProcessor> socket;
    tlm_utils::simple_initiator_socket<SoundProcessor> led_socket;
    
    int pulse_count;
    sc_time last_pulse_time;

    SC_CTOR(SoundProcessor) : pulse_count(0), last_pulse_time(SC_ZERO_TIME) {
        socket.register_b_transport(this, &SoundProcessor::process_sound);
    }

    void process_sound(tlm::tlm_generic_payload& trans, sc_time& delay) {
        int* signal = reinterpret_cast<int*>(trans.get_data_ptr());
        sc_time now = sc_time_stamp();
        sc_time interval = now - last_pulse_time;

        if (*signal == 1) {  // اگر پالس دریافت شد
            if (pulse_count == 0) {
                last_pulse_time = now;
                pulse_count++;
            } else {
                if (interval.to_seconds() >= 0.0001 && interval.to_seconds() <= 0.0002) {
                    pulse_count++;
                    if (pulse_count >= 3) {
                        send_led_signal(1, 0);  // LED تلفن را روشن کن
                    }
                } else {
                    send_led_signal(0, 1);  // LED در را روشن کن
                    pulse_count = 0;
                }
                last_pulse_time = now;
            }
        }
    }

    void send_led_signal(int phone_led, int door_led) {
        int led_status[2] = { phone_led, door_led };
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;

        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&led_status));
        trans.set_data_length(sizeof(led_status));
        led_socket->b_transport(trans, delay);
    }
};

// ---------------------------------------------
// ماژول کنترلر LED
// ---------------------------------------------
SC_MODULE(LEDController) {
    tlm_utils::simple_target_socket<LEDController> socket;

    SC_CTOR(LEDController) {
        socket.register_b_transport(this, &LEDController::update_leds);
    }

    void update_leds(tlm::tlm_generic_payload& trans, sc_time& delay) {
        int* led_status = reinterpret_cast<int*>(trans.get_data_ptr());
        int phone_led = led_status[0];
        int door_led = led_status[1];

        std::cout << "LED_PHONE: " << phone_led << ", LED_DOOR: " << door_led << " at " << sc_time_stamp() << std::endl;
    }
};

// ---------------------------------------------
// تست سیستم
// ---------------------------------------------
SC_MODULE(Testbench) {
    SoundSensor* sensor;
    SoundProcessor* processor;
    LEDController* led_controller;

    SC_CTOR(Testbench) {
        sensor = new SoundSensor("SoundSensor");
        processor = new SoundProcessor("SoundProcessor");
        led_controller = new LEDController("LEDController");

        // اتصال ماژول‌ها
        sensor->socket.bind(processor->socket);
        processor->led_socket.bind(led_controller->socket);
    }

    ~Testbench() {
        delete sensor;
        delete processor;
        delete led_controller;
    }
};

// ---------------------------------------------
// اجرای شبیه‌سازی
// ---------------------------------------------
int sc_main(int argc, char* argv[]) {
    Testbench tb("Testbench");
    sc_start();
    return 0;
}
