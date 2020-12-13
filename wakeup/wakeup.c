#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>

static struct task_struct *thread;

static DEFINE_SPINLOCK(wakeup_lock);
static DECLARE_COMPLETION(setup_done);

static struct req {
        struct req *next;
        struct completion done;
        int err;
        const char *name;
} *requests;


static void wakeup_work_loop(void)
{
	while(!kthread_should_stop()) {
		spin_lock(&wakeup_lock);
		while(requests){
			struct req *req = requests;
			requests = NULL;
			spin_unlock(&wakeup_lock);
			while(req) {
				pr_info("from:%s\n", req->name);
				complete(&req->done);
				req = req->next;
			}
			spin_lock(&wakeup_lock);
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock(&wakeup_lock);
		schedule();
	}
}

static void wakeup_test_submit_req(struct req *req)
{
	init_completion(&req->done);

	spin_lock(&wakeup_lock);
	req->next = requests;
	requests = req;
	spin_unlock(&wakeup_lock);
	wake_up_process(thread);

	wait_for_completion(&req->done);
}

static int wakeup_test_setup(void *p)
{
	int err;

	*(int*)p = 0;
	err = 0;

	complete(&setup_done);
/*	err = do_something();  */
	wakeup_work_loop();

	return err;
}

static void wakeup_test_case(void)
{
	struct req req_one;
	struct req req_two;
		
	req_one.name = "test_req_one";
	req_two.name = "test_req_two";
	wakeup_test_submit_req(&req_one);
	wakeup_test_submit_req(&req_two);
}

static int  __init wakeup_test_init(void)
{
	int err;

	thread = kthread_run(wakeup_test_setup, &err, "wakeup_test");
	if (IS_ERR(thread)) {
		thread = NULL;
		err = PTR_ERR(thread);
	}
	wait_for_completion(&setup_done);

	if (err) {
		printk(KERN_ERR "wakeup_test: unable to create wakeup_test %i\n", err);
		return err;
	}

	wakeup_test_case();
	return 0;
}

static void __exit wakeup_test_exit(void)
{
	pr_info("%s\n", __func__);
	kthread_stop(thread);
}

module_init(wakeup_test_init);
module_exit(wakeup_test_exit);
MODULE_AUTHOR("Jeff Xie");
MODULE_LICENSE("GPL");

