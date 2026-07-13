#include <rtdevice.h>
#include <rtthread.h>

#define UART7_DEVICE_NAME    "uart7"
#define UART7_BAUD_RATE      1500000U

static int uart7_send_hello(void)
{
    static const char message[] = "helloworld\r\n";
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_device_t device;
    rt_size_t written;
    rt_err_t result;

    device = rt_device_find(UART7_DEVICE_NAME);
    if (device == RT_NULL)
    {
        rt_kprintf("uart7 test: device not found\n");
        return -RT_ENOSYS;
    }

    config.baud_rate = UART7_BAUD_RATE;
    result = rt_device_control(device, RT_DEVICE_CTRL_CONFIG, &config);
    if (result != RT_EOK)
    {
        rt_kprintf("uart7 test: configure failed (%d)\n", result);
        return result;
    }

    result = rt_device_open(device, RT_DEVICE_OFLAG_WRONLY);
    if (result != RT_EOK)
    {
        rt_kprintf("uart7 test: open failed (%d)\n", result);
        return result;
    }

    written = rt_device_write(device, 0, message, sizeof(message) - 1);
    rt_device_close(device);

    if (written != sizeof(message) - 1)
    {
        rt_kprintf("uart7 test: write failed (%d/%d)\n",
                   (int)written, (int)(sizeof(message) - 1));
        return -RT_EIO;
    }

    return RT_EOK;
}

static int uart7_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    return uart7_send_hello();
}
MSH_CMD_EXPORT(uart7_test, send helloworld through uart7);

static int uart7_test_init(void)
{
    return uart7_send_hello();
}
INIT_APP_EXPORT(uart7_test_init);
