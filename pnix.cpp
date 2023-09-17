#include <linux/init.h>
#include <linux/module.h>

Kernel features
* Process virtualization / scheduling
* Filesystem
* Virtual memory management

Kernel module - for mounting in a host OS
Daemon - core logic, loads subfile handlers, keeps track of VFS

struct dentry* lookup(struct inode* ino, struct dentry* dent, unsigned int mode) {
	
}

static int __init aufs_init() {
	pr_debug("aufs module loaded\n");
	return 0;
}

static void __exit aufs_fini() {
	pr_debug("aufs module unloaded\n");
}

static void aufs_put_super(struct super_block *sb)
  {
      pr_debug("aufs super block destroyed\n");
  }

  static struct super_operations const aufs_super_ops = {
      .put_super = aufs_put_super,
  };

struct super_operations xfs_super_ops = {
	
	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);

	void (*dirty_inode) (struct inode *, int flags);
	int (*write_inode) (struct inode *, int);
	void (*drop_inode) (struct inode *);
	void (*delete_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
	int (*freeze_fs) (struct super_block *);
	int (*unfreeze_fs) (struct super_block *);
	int (*statfs) (struct dentry *, struct kstatfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
	void (*clear_inode) (struct inode *);
	void (*umount_begin) (struct super_block *);

	int (*show_options)(struct seq_file *, struct dentry *);

	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
	int (*nr_cached_objects)(struct super_block *);
	void (*free_cached_objects)(struct super_block *, int);
}

static int aufs_fill_sb(struct super_block* sb, void* data, int silent) {
	struct inode* root = NULL;
	struct file* f;
	qstr x;
	
	sb->s_magic = AUFS_MAGIC_NUMBER;
	sb->s_op = &aufs_super_ops;

	root = new_inode(sb);
	if (!root) {
		pr_err("inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_ino = 0;
	root->i_sb = sb;
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
	inode_init_owner(root, NULL, S_IFDIR);

	sb->s_root = d_make_root(root);
	if (!sb->s_root)
	{
		pr_err("root creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

static struct dentry* pnixfs_mount(struct file_system_type* type, int flags, char const* dev, void* data) {
	return mount_nodev(type, flags, data, aufs_fill_sb);
}

static struct file_system_type aufs_type = {
	.name = "pnixfs",
	.fs_flags = FS_REQUIRES_DEV,
	.mount = pnix_mount,
	.kill_sb = kill_block_super,
	.owner = THIS_MODULE
};

module_init(aufs_init);
module_exit(aufs_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kmu");

/*
 * Resizable simple ram filesystem for Linux.
 *
 * Copyright (C) 2000 Linus Torvalds.
 *               2000 Transmeta Corp.
 *
 * Usage limits added by David Gibson, Linuxcare Australia.
 * This file is released under the GPL.
 */

/*
 * NOTE! This filesystem is probably most useful
 * not as a real filesystem, but as an example of
 * how virtual filesystems can be written.
 *
 * It doesn't get much simpler than this. Consider
 * that this file implements the full semantics of
 * a POSIX-compliant read-write filesystem.
 *
 * Note in particular how the filesystem does not
 * need to implement any data structures of its own
 * to keep track of the virtual data: using the VFS
 * caches is sufficient.
 */

#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/pnixfs.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>

#include <linux/fs.h>
#include <linux/pagemap.h>

#define PNIX_MAGIC 0xfef0c6cf /* random */

struct pnixfs_mount_opts {
	umode_t mode;
};

struct pnixfs_fs_info {
	struct pnixfs_mount_opts mount_opts;
};

#define pnixfs_DEFAULT_MODE	0755

static const struct super_operations pnixfs_ops;
static const struct inode_operations pnixfs_dir_inode_operations;

static const struct address_space_operations pnixfs_aops = {
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
	.set_page_dirty	= __set_page_dirty_no_writeback,
};

struct inode *pnixfs_get_inode(struct super_block *sb,
				const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &pnixfs_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
		switch (mode & S_IFMT) {
		default:
			init_special_inode(inode, mode, dev);
			break;
		case S_IFREG:
			inode->i_op = &pnixfs_file_inode_operations;
			inode->i_fop = &pnixfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &pnixfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			break;
		}
	}
	return inode;
}

/*
 * File creation. Allocate an inode, and we're done..
 */
/* SMP-safe */
static int
pnixfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode * inode = pnixfs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = current_time(dir);
	}
	return error;
}

static int pnixfs_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)
{
	int retval = pnixfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

static int pnixfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return pnixfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int pnixfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	struct inode *inode;
	int error = -ENOSPC;

	inode = pnixfs_get_inode(dir->i_sb, dir, S_IFLNK|S_IRWXUGO, 0);
	if (inode) {
		int l = strlen(symname)+1;
		error = page_symlink(inode, symname, l);
		if (!error) {
			d_instantiate(dentry, inode);
			dget(dentry);
			dir->i_mtime = dir->i_ctime = current_time(dir);
		} else
			iput(inode);
	}
	return error;
}

static const struct inode_operations pnixfs_dir_inode_operations = {
	.create		= pnixfs_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= pnixfs_symlink,
	.mkdir		= pnixfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= pnixfs_mknod,
	.rename		= simple_rename,
	
	struct dentry *(* 	lookup )(struct inode *, struct dentry *, unsigned int)
 
void *(* 	follow_link )(struct dentry *, struct nameidata *)
 
int(* 	permission )(struct inode *, int)
 
struct posix_acl *(* 	get_acl )(struct inode *, int)
 
int(* 	readlink )(struct dentry *, char __user *, int)
 
void(* 	put_link )(struct dentry *, struct nameidata *, void *)
 
int(* 	create )(struct inode *, struct dentry *, umode_t, bool)
 
int(* 	link )(struct dentry *, struct inode *, struct dentry *)
 
int(* 	unlink )(struct inode *, struct dentry *)
 
int(* 	symlink )(struct inode *, struct dentry *, const char *)
 
int(* 	mkdir )(struct inode *, struct dentry *, umode_t)
 
int(* 	rmdir )(struct inode *, struct dentry *)
 
int(* 	mknod )(struct inode *, struct dentry *, umode_t, dev_t)
 
int(* 	rename )(struct inode *, struct dentry *, struct inode *, struct dentry *)
 
void(* 	truncate )(struct inode *)
 
int(* 	setattr )(struct dentry *, struct iattr *)
 
int(* 	getattr )(struct vfsmount *mnt, struct dentry *, struct kstat *)
 
int(* 	setxattr )(struct dentry *, const char *, const void *, size_t, int)
 
ssize_t(* 	getxattr )(struct dentry *, const char *, void *, size_t)
 
ssize_t(* 	listxattr )(struct dentry *, char *, size_t)
 
int(* 	removexattr )(struct dentry *, const char *)
 
int(* 	fiemap )(struct inode *, struct fiemap_extent_info *, u64 start, u64 len)
 
int(* 	update_time )(struct inode *, struct timespec *, int)
 
int(* 	atomic_open )(struct inode *, struct dentry *, struct file *, unsigned open_flag, umode_t create_mode, int *opened)
};

/*
 * Display the mount options in /proc/mounts.
 */
static int pnixfs_show_options(struct seq_file *m, struct dentry *root)
{
	struct pnixfs_fs_info *fsi = root->d_sb->s_fs_info;

	if (fsi->mount_opts.mode != pnixfs_DEFAULT_MODE)
		seq_printf(m, ",mode=%o", fsi->mount_opts.mode);
	return 0;
}

static const struct super_operations pnixfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= pnixfs_show_options,
};

enum pnixfs_param {
	Opt_mode,
};

const struct fs_parameter_spec pnixfs_fs_parameters[] = {
	fsparam_u32oct("mode",	Opt_mode),
	{}
};

static int pnixfs_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct fs_parse_result result;
	struct pnixfs_fs_info *fsi = fc->s_fs_info;
	int opt;

	opt = fs_parse(fc, pnixfs_fs_parameters, param, &result);
	if (opt < 0) {
		/*
		 * We might like to report bad mount options here;
		 * but traditionally pnixfs has ignored all mount options,
		 * and as it is used as a !CONFIG_SHMEM simple substitute
		 * for tmpfs, better continue to ignore other mount options.
		 */
		if (opt == -ENOPARAM)
			opt = 0;
		return opt;
	}

	switch (opt) {
	case Opt_mode:
		fsi->mount_opts.mode = result.uint_32 & S_IALLUGO;
		break;
	}

	return 0;
}

static int pnixfs_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct pnixfs_fs_info *fsi = sb->s_fs_info;
	struct inode *inode;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= pnixfs_MAGIC;
	sb->s_op		= &pnixfs_ops;
	sb->s_time_gran		= 1;

	inode = pnixfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static int pnixfs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, pnixfs_fill_super);
}

static void pnixfs_free_fc(struct fs_context *fc)
{
	kfree(fc->s_fs_info);
}

static const struct fs_context_operations pnixfs_context_ops = {
	.free		= pnixfs_free_fc,
	.parse_param	= pnixfs_parse_param,
	.get_tree	= pnixfs_get_tree,
};

int pnixfs_init_fs_context(struct fs_context *fc)
{
	struct pnixfs_fs_info *fsi;

	fsi = kzalloc(sizeof(*fsi), GFP_KERNEL);
	if (!fsi)
		return -ENOMEM;

	fsi->mount_opts.mode = pnixfs_DEFAULT_MODE;
	fc->s_fs_info = fsi;
	fc->ops = &pnixfs_context_ops;
	return 0;
}

static void pnixfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type pnixfs_fs_type = {
	.name		= "pnixfs",
	.init_fs_context = pnixfs_init_fs_context,
	.parameters	= pnixfs_fs_parameters,
	.kill_sb	= pnixfs_kill_sb,
	.fs_flags	= FS_USERNS_MOUNT,
	
};

static int __init init_pnixfs_fs(void)
{
	return register_filesystem(&pnixfs_fs_type);
}
fs_initcall(init_pnixfs_fs);