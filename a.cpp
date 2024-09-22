#include <bits/stdc++.h>
using namespace std;

int main() {
    int n;
    cin >> n;
    
    vector<vector<int>> a(n, vector<int>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i+1; j++) {
            cin >> a[i][j];
        }
    }

    int ans = 1;
    for (int i = 0; i < n; ++i) {
        if (i >= j)
        ans = a[i][j];
    else
        ans = a[j][i];
    }
    cout << ans << endl;
    return 0;
}