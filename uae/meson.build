specific_ss.add(when: 'CONFIG_SOFTMMU', if_true: files(
  'qemu-uae-cpu.c',
  'qemu-uae-main.c',
  'qemu-uae-stubs.c',
))
specific_ss.add(when: 'CONFIG_UAE_TEST', if_true: files('test_harness.c'))
