.ONESHELL: # enables `cd` inside a target to work normally https://stackoverflow.com/a/30590240/4692662

build: nginx/objs/nginx nginx/objs/pathological_module.so

nginx/objs/nginx:
	cd nginx
	./auto/configure --with-compat --add-dynamic-module=..
	make

nginx/objs/pathological_module.so: src/pathological.c
	cd nginx
	make modules

start: nginx/objs/nginx nginx/objs/pathological_module.so
	./nginx/objs/nginx -p nix -e stderr

clean:
	rm -rf nginx/objs

test: nginx/objs/nginx nginx/objs/pathological_module.so
	TEST_NGINX_LOAD_MODULES=../../nginx/objs/pathological_module.so TEST_NGINX_BINARY=./nginx/objs/nginx prove -I. -r t
