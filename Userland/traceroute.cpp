/*
 * Copyright (c) 2020, the SerenityOS developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <LibCore/ArgsParser.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static uint16_t internet_checksum(const void* ptr, size_t count)
{
    uint32_t checksum = 0;
    auto* w = (const uint16_t*)ptr;
    while (count > 1) {
        checksum += ntohs(*w++);
        if (checksum & 0x80000000)
            checksum = (checksum & 0xffff) | (checksum >> 16);
        count -= 2;
    }
    while (checksum >> 16)
        checksum = (checksum & 0xffff) + (checksum >> 16);
    return htons(~checksum);
}

int main(int argc, char** argv) 
{
    if (pledge("stdio id inet dns", nullptr) < 0) {
        printf("A\n");
        perror("pledge");
        return 1;
    }

    const char* target = nullptr;
    int max_hops = 255;
    int timeout_ms = 5000;

    Core::ArgsParser args_parse;
    args_parse.add_positional_argument(target, "Target", "target");
    args_parse.add_option(max_hops, "Maximum number of hops to target", "maximum_hops", 'm', "maximum_hops");
    args_parse.add_option(timeout_ms, "Timeout for reply wait, in milliseconds", "timeout", 'w', "timeout");
    args_parse.parse(argc, argv, true);

    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if(sock_fd < 0) {
        perror("socket");
        return 1;
    }

    if (setgid(getgid()) || setuid(getuid())) {
        fprintf(stderr, "Failed to drop privileges.\n");
        return 1;
    }

    if (pledge("stdio inet dns", nullptr) < 0) {
        printf("B\n");
        perror("pledge");
        return 1;
    }

    timeval timeout {
        (time_t) (timeout_ms / 1000),
        (timeout_ms % 1000) * 1000
    };

    int result = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (result < 0) {
        perror("setsockopt");
        close(sock_fd);
        return 1;
    }

    auto* target_host = gethostbyname(target);

    if(!target_host) {
        fprintf(stderr, "Lookup failed for %s\n", target);
        close(sock_fd);
        return 1;
    }

    uint32_t target_inaddr = *(const in_addr_t*)target_host->h_addr_list[0];

    if (pledge("stdio inet", nullptr) < 0) {
        printf("C\n");
        perror("pledge");
        close(sock_fd);
        return 1;
    }

    pid_t pid = getpid();

    sockaddr_in target_address;
    memset(&target_address, 0, sizeof(target_address));
    target_address.sin_family = AF_INET;
    target_address.sin_port = 0;

    target_address.sin_addr.s_addr = target_inaddr;

    sockaddr_in recv_address;

    struct PingPacket {
        struct icmphdr header;
        char msg[64 - sizeof(struct icmphdr)];
    };

    int seq = 1;

    for( ; (seq - 1) < max_hops; seq++ ) {
        PingPacket ping_packet;
        PingPacket pong_packet;
        memset(&ping_packet, 0, sizeof(PingPacket));
        memcpy(&recv_address, &target_address, sizeof(target_address));

        result = setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &seq, sizeof(seq));
        if (result < 0) {
            perror("setsockopt");
            close(sock_fd);
            return 1;
        }

        ping_packet.header.type = 8;
        ping_packet.header.code = 0;
        ping_packet.header.un.echo.id = htons(pid);
        ping_packet.header.un.echo.sequence = htons(seq);
        ping_packet.header.checksum = internet_checksum(&ping_packet, sizeof(PingPacket));

        result = sendto(sock_fd, &ping_packet, sizeof(PingPacket), 0, (const struct sockaddr*)&target_address, sizeof(sockaddr_in));
        if (result < 0) {
            perror("sendto");
            close(sock_fd);
            return 1;
        }
        socklen_t recv_addr_size = sizeof(recv_address);
        result = recvfrom(sock_fd, &pong_packet, sizeof(PingPacket), 0, (struct sockaddr*)&recv_address, &recv_addr_size);
        if (result < 0) {
            perror("recvfrom");
            close(sock_fd);
            return 1;
        }

        if (pong_packet.header.type == 11 || pong_packet.header.type == 0) {
            char ntop_buf[64];
            inet_ntop(AF_INET, &recv_address.sin_addr, ntop_buf, sizeof(ntop_buf));
            printf("hop %d - %s\n", seq, ntop_buf);
        }

        if(pong_packet.header.type == 0) {
            printf("done\n");
            close(sock_fd);
            return 0;
        }
    }

    printf("Exceeded maximum hop count before destination was reached.\n");
    close(sock_fd);
    return 1;
}