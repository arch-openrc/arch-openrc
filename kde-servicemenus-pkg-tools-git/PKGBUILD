# Maintainer: udeved <udeved@openrc4arch.site40.net>

pkgname=kde-servicemenus-pkg-tools-git
pkgver=0.r6.d40a0bb
pkgrel=1
pkgdesc="Kde servicemenu for makepkg, repo-add & repo-remove, anmcap & aur upload."
arch=('any')
url="https://github.com/udeved/kde-servicemenus-pkg-tools"
license=('GPL2')
depends=('kdebase-dolphin' 'pacman' 'namcap' 'curl')
makedepends=('git')
conflicts=('kde-servicemenus-pkg-tools')
install=pkg-tools.install
source=("$pkgname"::'git://github.com/udeved/kde-servicemenu-pkg-tools.git')
md5sums=('SKIP')

pkgver() {
      cd "$srcdir/$pkgname"
      printf "0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

package() {
      cd "$pkgname"

      install -Dm755 bin/pkg-magic $pkgdir/usr/bin/pkg-magic

      install -Dm644  ServiceMenus/repo-action.desktop $pkgdir/usr/share/kde4/services/ServiceMenus/repo-action.desktop
      install -Dm644  ServiceMenus/makepkg.desktop $pkgdir/usr/share/kde4/services/ServiceMenus/makepkg.desktop
      install -Dm644  ServiceMenus/pacman.desktop $pkgdir/usr/share/kde4/services/ServiceMenus/pacman.desktop
      install -Dm644  ServiceMenus/namcap.desktop $pkgdir/usr/share/kde4/services/ServiceMenus/namcap.desktop
      install -Dm644  ServiceMenus/aur-upload.desktop $pkgdir/usr/share/kde4/services/ServiceMenus/aur-upload.desktop

      install -Dm644 mime/application/x-xz-pkg.xml $pkgdir/usr/share/mime/application/x-xz-pkg.xml
      install -Dm644 mime/application/x-pkgbuild.xml $pkgdir/usr/share/mime/application/x-pkgbuild.xml
      install -Dm644 mime/packages/application-x-xz-pkg.xml $pkgdir/usr/share/mime/packages/application-x-xz-pkg.xml
      install -Dm644 mime/packages/application-x-pkgbuild.xml $pkgdir/usr/share/mime/packages/application-x-pkgbuild.xml
      install -Dm644 mime/application/x-gz-src.xml $pkgdir/usr/share/mime/application/x-gz-src.xml
      install -Dm644 mime/packages/application-x-gz-src.xml $pkgdir/usr/share/mime/packages/application-x-gz-src.xml

}
