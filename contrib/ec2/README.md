Running on EC2
======================

[![Code Quality](https://landscape.io/github/eddyxu/ec2/master/landscape.png)](https://landscape.io/github/eddyxu/ec2/master)

1. Setup
----------------------

1.1 Setup Amazon AWS account

Exporting the following environment variables in `~/.bash_profile` or `~/.zshrc`:
```sh
export AWS_ACCESS_KEY_ID=xxxxx
export AWS_SECRET_ACCESS_KEY=xxxxx
export AWS_ACCOUNT_ID=xxxxx
```

Creates `X.509` credentials and download them (a `cert-xxxxx.pem` file and a `pk-xxxx.pem` file) into `~/.ec2/` directories.

After that, please go to `AWS Web Console` -> `EC2 Dashboard` -> `Key Pairs`, and creates a Amazon EC2 public key, and stores it (e.g., `foo.pem`) to `~/.ssh/ec2.pem`.

2. How to run this project
----------------------------

2.1 Create the virtual environment and install dependancies (once).

```sh
cd /path/to/project
# Only run once of virtual env
virtualenv env
. env/bin/activate
pip install -r requirements.txt
```

2.2 Run `fabric`.

```sh
fab -l   # show all available operations.
```
