
#define MAX_HUE_VALUE 31000
#define MAX_SATURATION_VALUE 255
#define MAX_VALUE_VALUE 255

void hsv2rgb(uint16_t h, uint32_t s, uint32_t v, int color[3])
{

    if (h >= MAX_HUE_VALUE)
        h = MAX_HUE_VALUE;
    int16_t ph = h / 6;
    ph = ph % 2000;
    ph -= 1000;
    ph = 1000 - abs(ph);
    if (h == 0)
        ph = 0;
    uint32_t c = v * s;
    uint32_t x2 = c * ph;
    uint8_t x = x2 / 255000;
    uint8_t r, g, b;
    int32_t m = v * (255 - s) / 255;
    x += m;
    if (0 <= h && h < 6000)
    {
        r = v;
        g = x;
        b = m;
    } //rgb = [c, x, 0];
    if (6000 <= h && h < 12000)
    {
        r = x;
        g = v;
        b = m;
    } //rgb = [x, c, 0];
    if (12000 <= h && h < 18000)
    {
        r = m;
        g = v;
        b = x;
    } //rgb = [0, c, x];
    if (18000 <= h && h < 24000)
    {
        r = m;
        g = x;
        b = v;
    } //rgb = [0, x, c];
    if (24000 <= h && h < 30000)
    {
        r = x;
        g = m;
        b = v;
    } //rgb = [x, 0, c];
    if (30000 <= h && h < 36000)
    {
        r = v;
        g = m;
        b = x;
    } //rgb = [c, 0, x];

    color[0] = r;
    color[1] = g;
    color[2] = b;
}