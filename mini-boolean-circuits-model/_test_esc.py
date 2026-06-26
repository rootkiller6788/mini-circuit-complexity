s1 = 'test\nhere'
s2 = r'test\nhere'
print('s1 repr:', repr(s1), 'len:', len(s1))
print('s2 repr:', repr(s2), 'len:', len(s2))
with open('_test_esc_out.txt', 'w') as f:
    f.write(s1 + '\n')
    f.write(s2 + '\n')
