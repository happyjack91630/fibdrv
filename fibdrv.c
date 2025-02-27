#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 500
#define max_space 300

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

ktime_t kt;
ssize_t sequence_result, doubling_result;

static void sum(char *a, char *b, char *c)
{
    int last_pos = max_space - 1;
    int carry = 0;
    int i = max_space - 1, j = max_space - 1;
    for (; j >= 0; i--, j--) {
        int sum_v;
        sum_v = (a[i] - '0') + (b[j] - '0') + carry;
        carry = sum_v / 10;
        sum_v %= 10;
        c[last_pos--] = sum_v + '0';
    }
}

static void int2char(long long a, long long b, char *a_s, char *b_s)
{
    int reminder;
    int last_pos_a = max_space - 1;
    int last_pos_b = max_space - 1;
    while (a > 0) {
        reminder = a % 10;
        a_s[last_pos_a--] = reminder + '0';
        a = a / 10;
    }
    while (b > 0) {
        reminder = b % 10;
        b_s[last_pos_b--] = reminder + '0';
        b = b / 10;
    }
}

static void fib_big_num(int n, char *r)
{
    if (n == 0) {
        *r = n + '0';
        r++;
        *r = '\0';
        return;
    }
    if (n == 1 || n == 2) {
        *r = '1';
        r++;
        *r = '\0';
        return;
    }

    long long a = 1;
    long long b = 1;
    char a_s[max_space];
    char b_s[max_space];
    char ans[max_space];
    for (int i = 0; i < max_space; i++) {
        a_s[i] = '0';
        b_s[i] = '0';
        ans[i] = '0';
    }
    int2char(a, b, a_s, b_s);
    for (int i = 3; i <= n; i++) {
        sum(a_s, b_s, ans);
        for (int j = 0; j < max_space; j++) {
            a_s[j] = b_s[j];
            b_s[j] = ans[j];
        }
    }

    int flag = 0;
    for (int i = 0; i < max_space; i++) {
        if (ans[i] == '0' && flag == 0) {
            continue;
        } else {
            flag = 1;
            *r++ = ans[i];
        }
    }
    *r = '\0';
}

static long long fib_time_proxy(long long k, long long (*func_ptr)(long long))
{
    kt = ktime_get();
    long long result = func_ptr(k);
    kt = ktime_sub(ktime_get(), kt);
    return result;
}

static long long fib_sequence(long long k)
{
    if (k < 2)
        return 0;

    long long a = 0, b = 1;
    for (int i = 2; i <= k; i++) {
        long long c = a + b;
        a = b;
        b = c;
    }

    return b;
}

static long long fib_doubling(long long k)
{
    unsigned int h = 0;
    for (unsigned int i = k; i; ++h, i >>= 1)
        ;

    long long a = 0;  // F(0) = 0
    long long b = 1;  // F(1) = 1
    // There is only one `1` in the bits of `mask`. The `1`'s position is same
    // as the highest bit of n(mask = 2^(h-1) at first), and it will be shifted
    // right iteratively to do `AND` operation with `n` to check `n_j` is odd or
    // even, where n_j is defined below.
    for (unsigned int mask = 1 << (h - 1); mask; mask >>= 1) {  // Run h times!
        // Let j = h-i (looping from i = 1 to i = h), n_j = floor(n / 2^j) = n
        // >> j (n_j = n when j = 0), k = floor(n_j / 2), then a = F(k), b =
        // F(k+1) now.
        long long c = a * (2 * b - a);  // F(2k) = F(k) * [ 2 * F(k+1) – F(k) ]
        long long d = a * a + b * b;    // F(2k+1) = F(k)^2 + F(k+1)^2

        if (mask & k) {  // n_j is odd: k = (n_j-1)/2 => n_j = 2k + 1
            a = d;       //   F(n_j) = F(2k + 1)
            b = c + d;   //   F(n_j + 1) = F(2k + 2) = F(2k) + F(2k + 1)
        } else {         // n_j is even: k = n_j/2 => n_j = 2k
            a = c;       //   F(n_j) = F(2k)
            b = d;       //   F(n_j + 1) = F(2k + 1)
        }
    }
    return a;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}



/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    printk("===============%lld============", *offset);
    char result[max_space];
    fib_big_num(*offset, result);
    size_t len = strlen(result) + 1;
    size_t left = copy_to_user(buf, result, len);
    return left;
}


/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t mode,
                         loff_t *offset)
{
    switch (mode) {
    case 0:
        sequence_result = fib_time_proxy(*offset, fib_sequence);
        printk("%lld", kt);
        break;
    case 1:
        doubling_result = fib_time_proxy(*offset, fib_doubling);
        printk("%lld", kt);
        break;
    case 2:
        return 1;
    }
    return (ssize_t) ktime_to_ns(kt);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
