#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>

enum
{
	EN_listener_port = 30000,
};

int main()
{
	enum
	{
		EN_pcs_backlog_for_listen = 10,
		EN_max_pcs_epoll_events = 10,
	};

	std::cout << "--- 実行開始 / ポート番号 -> " << EN_listener_port << std::endl;

	// リスナ用のソケットを生成する
	// 第３引数の 0 は、デフォルトのプロトコルを使用することを示す（この場合は TCP）
	// UDP を利用する場合は、第２引数が SOCK_DGRAM となる
	const int fd_sock_listner = socket(AF_INET6, SOCK_STREAM, 0);
	if (fd_sock_listner < 0)
	{
		std::cout << "!!! fd_sock_listner の生成に失敗しました。" << std::endl;
		return -1;
	}
	std::cout << "--- fd_sock_listner を生成しました。" << std::endl;

	// SO_REUSEADDR の指定
	{
		int opt_val = 1;  // SO_REUSEADDR を利用するときには１を設定する
		if (setsockopt(fd_sock_listner, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) < 0)
		{
			std::cout << "!!! fd_sock_listner -> setsockopt() が失敗しました。" << std::endl;
			return -1;
		}
	}
	std::cout << "--- fd_sock_listner に SO_REUSEADDR を設定しました。" << std::endl;
	
	// バインド
	{
		sockaddr_in6 addr_listener = {};
		addr_listener.sin6_family = AF_INET6;
		addr_listener.sin6_port = htons(EN_listener_port);  // ネットワークバイトオーダーへ変換
		addr_listener.sin6_addr = in6addr_loopback;  // in6addr_loopback =「::1」
//		addr_listener.sin6_addr = in6addr_any;  // in6addr_any =「::」

		if (bind(fd_sock_listner, (sockaddr*)&addr_listener, sizeof(addr_listener)) < 0)
		{
			std::cout << "!!! fd_sock_listner -> bind() が失敗しました。" << std::endl;
			return -1;
		}
	}
	std::cout << "--- bind しました。" << std::endl;
	
	// listen
	if (listen(fd_sock_listner, EN_pcs_backlog_for_listen) < 0)
	{
		std::cout << "!!! fd_sock_listner -> listen() が失敗しました。" << std::endl;
		return -1;
	}
	std::cout << "--- listen を開始します。" << std::endl;

	// fd_epoll の生成
	const int fd_epoll = epoll_create1(0);  // 引数は flags
	if (fd_epoll < 0)
	{
		std::cout << "!!! fd_epoll -> epoll_create1() が失敗しました。" << std::endl;
		close(fd_sock_listner);
		return -1;
	}
	std::cout << "--- fd_epoll を生成しました。" << std::endl;

	// fd_epoll に fd_sock_listner を登録
	{
		epoll_event e;
		e.events = EPOLLIN;
		e.data.ptr = NULL;	// 本来は必要なアドレスを設定する
	
		if (epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_sock_listner, &e) < 0)
		{
			std::cout << "!!! fd_epoll -> epoll_ctl() が失敗しました。" << std::endl;
			close(fd_sock_listner);
			close(fd_epoll);
			return -1;
		}
		std::cout << "--- fd_epoll に fd_sock_listner を登録しました。" << std::endl;
	}
	
	// 接続待機
	epoll_event ary_events[EN_max_pcs_epoll_events];
	const int ret_epoll_wait = epoll_wait(fd_epoll, ary_events, EN_max_pcs_epoll_events
		, -1	// timeout（ミリ秒）/ -1 を指定すると無制限に待機する
	);
	std::cout << "--- epoll_wait から抜けました。" << std::endl;

	// 終了処理
	if (close(fd_sock_listner) < 0)
	{
		std::cout << "!!! リスナ用のソケットの close に失敗しました。" << std::endl;
	}
	else
	{
		std::cout << "--- fd_sock_listner を close しました。" << std::endl;
	}

	if (close(fd_epoll) < 0)
	{
		std::cout << "!!! fd_epoll の close に失敗しました。" << std::endl;
	}
	else
	{
		std::cout << "--- fd_epoll を close しました。" << std::endl;
	}

	return 0;
}
