static inline bool per_cpu_enqueue(struct usk_poll_thread_struct *poll, void *val)
{
	struct per_cpu_queue *q = this_cpu_ptr(poll->tx_pending_queue);
	u32 w = q->write;
	u32 r = READ_ONCE(q->read);

	if ((u32)(w - r) >= TX_PENDING_LIST_SIZE) {
		/* queue is full */
		return false;
	}

	q->addr[w & (TX_PENDING_LIST_SIZE - 1)] = val;
	/* ensure write data before update index */
	smp_wmb();
	WRITE_ONCE(q->write, w + 1);

	return true;
}

static inline bool per_cpu_dequeue(struct usk_poll_thread_struct *poll, int cpu, void **val)
{
	struct per_cpu_queue *q = per_cpu_ptr(poll->tx_pending_queue, cpu);
	u32 r = q->read;
	u32 w = READ_ONCE(q->write);

	if (r == w)
		return false; /* empty */

	*val = q->addr[r & (TX_PENDING_LIST_SIZE - 1)];
	/* ensure read data before update index */
	smp_rmb();
	WRITE_ONCE(q->read, r + 1);

	return true;
}
