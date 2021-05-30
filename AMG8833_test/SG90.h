#ifndef SG90_H
#define SG90_H

#define MIN_ANGLE_BIT   (26)
#define MAX_ANGLE_BIT   (123)

class SG90 {
public:
    SG90(void) {};
    virtual ~SG90(void) {};
    void begin(int pin, int ch, int min_angle = -90, int max_angle = 90);
    void write(int angle); // from -90 to +90
    void move(int angle);
private:
    int _ch;
    int _angle;
    int _min = MIN_ANGLE_BIT;  // (26/1024)*20ms ≒ 0.5 ms  (-90°)
    int _max = MAX_ANGLE_BIT; // (123/1024)*20ms ≒ 2.4 ms (+90°)
};

#endif // SG90_H
