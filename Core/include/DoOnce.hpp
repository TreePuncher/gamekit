#include <atomic>

namespace FlexKit
{
	template<typename TY_OP>
	struct MT_DoOnce
	{
		template<typename ... TY_args>
		void operator () (TY_args&& ... args)requires requires(TY_OP op, TY_args ... args)
		{
			op(args...);
		}
		{
			if (!completed.load(std::memory_order_relaxed))
			{
				if (auto res = lock.fetch_add(1, std::memory_order_acq_rel); res != 0)
					while (!completed.load(std::memory_order_acquire));
				else
				{
					op(std::forward<TY_args>(args)...);
					completed = true;
				}
			}
		}

		std::atomic_int		lock = 0;
		std::atomic_bool	completed = false;
		TY_OP				op;
	};


	template<typename TY_op>
	auto MakeDoOnce(TY_op fnOperation)
	{
		return MT_DoOnce<TY_op>{
			.op = fnOperation
		};
	}
}