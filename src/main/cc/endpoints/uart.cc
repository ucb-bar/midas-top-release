#include "uart.h"

uart_t::uart_t(simif_t* sim): endpoint_t(sim)
{
}

void uart_t::send() {
  /* TODO:
  if (data.in.fire()) {
    write(UARTWIDGET_0(in_bits), data.in.bits);
    write(UARTWIDGET_0(in_valid), data.in.valid);
  }
  */
  if (data.out.fire()) {
    write(UARTWIDGET_0(out_ready), data.out.ready);
  }
}

void uart_t::recv() {
  // TODO: data.in.ready = read(UARTWIDGET_0(in_ready));
  data.out.valid = read(UARTWIDGET_0(out_valid));
  if (data.out.valid) {
    data.out.bits = read(UARTWIDGET_0(out_bits));
  }
}

void uart_t::tick() {
  data.out.ready = true;
  do {
    this->recv();

    // TODO: handle inputs
    data.in.valid = false;
    data.in.bits = 0;

    if (data.out.fire()) {
      fprintf(stdout, "%c", data.out.bits);
    }

    this->send();
  } while(data.in.fire() || data.out.fire());
}
